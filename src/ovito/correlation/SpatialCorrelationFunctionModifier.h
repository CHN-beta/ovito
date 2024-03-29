////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 OVITO GmbH, Germany
//  Copyright 2017 Lars Pastewka
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#pragma once


#include <ovito/particles/Particles.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/properties/PropertyObject.h>
#include <ovito/stdobj/table/DataTable.h>
#include <ovito/particles/util/CutoffNeighborFinder.h>
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/core/dataset/pipeline/AsynchronousModifier.h>

#include <complex>

namespace Ovito::Particles {

/**
 * \brief This modifier computes the spatial correlation function between two particle properties.
 */
class OVITO_CORRELATIONFUNCTIONPLUGIN_EXPORT SpatialCorrelationFunctionModifier : public AsynchronousModifier
{
	/// Give this modifier class its own metaclass.
	class OOMetaClass : public AsynchronousModifier::OOMetaClass
	{
	public:

		/// Inherit constructor from base metaclass.
		using AsynchronousModifier::OOMetaClass::OOMetaClass;

		/// Asks the metaclass whether the modifier can be applied to the given input data.
		virtual bool isApplicableTo(const DataCollection& input) const override;
	};

	OVITO_CLASS_META(SpatialCorrelationFunctionModifier, OOMetaClass)

	Q_CLASSINFO("ClassNameAlias", "CorrelationFunctionModifier");
	Q_CLASSINFO("DisplayName", "Spatial correlation function");
#ifndef OVITO_QML_GUI
	Q_CLASSINFO("ModifierCategory", "Analysis");
#else
	Q_CLASSINFO("ModifierCategory", "-");
#endif

public:

    enum AveragingDirectionType {
		CELL_VECTOR_1 = 0,
		CELL_VECTOR_2 = 1,
		CELL_VECTOR_3 = 2,
		RADIAL = 3
	};
    Q_ENUM(AveragingDirectionType);

    enum NormalizationType {
		VALUE_CORRELATION = 0,
		DIFFERENCE_CORRELATION = 1
	};
    Q_ENUM(NormalizationType);

	/// Constructor.
	Q_INVOKABLE SpatialCorrelationFunctionModifier(ObjectCreationParams params);

	/// This method is called by the system after the modifier has been inserted into a data pipeline.
	virtual void initializeModifier(const ModifierInitializationRequest& request) override;

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<EnginePtr> createEngine(const ModifierEvaluationRequest& request, const PipelineFlowState& input) override;

private:

	/// Computes the modifier's results.
	class CorrelationAnalysisEngine : public Engine
	{
	public:

		/// Constructor.
		CorrelationAnalysisEngine(const ModifierEvaluationRequest& request,
								  ConstPropertyPtr positions,
								  ConstPropertyPtr sourceProperty1,
								  size_t vecComponent1,
								  ConstPropertyPtr sourceProperty2,
								  size_t vecComponent2,
								  const SimulationCellObject* simCell,
								  FloatType fftGridSpacing,
								  bool applyWindow,
								  bool doComputeNeighCorrelation,
								  FloatType neighCutoff,
								  int numberOfNeighBins,
								  AveragingDirectionType averagingDirection) :
			Engine(request),
			_positions(std::move(positions)),
			_sourceProperty1(std::move(sourceProperty1)), _vecComponent1(vecComponent1),
			_sourceProperty2(std::move(sourceProperty2)), _vecComponent2(vecComponent2),
			_simCell(simCell), _fftGridSpacing(fftGridSpacing),
			_applyWindow(applyWindow), _neighCutoff(neighCutoff),
			_averagingDirection(averagingDirection),
			_neighCorrelation(doComputeNeighCorrelation ? DataTable::OOClass().createUserProperty(request.dataset(), numberOfNeighBins, PropertyObject::Float, 1, tr("Neighbor C(r)"), DataBuffer::InitializeMemory) : nullptr) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void applyResults(const ModifierEvaluationRequest& request, PipelineFlowState& state) override;

		/// This method is called by the system whenever a parameter of the modifier changes.
		/// The method can be overridden by subclasses to indicate to the caller whether the engine object should be 
		/// discarded (false) or may be kept in the cache, because the computation results are not affected by the changing parameter (true). 
		virtual bool modifierChanged(const PropertyFieldEvent& event) override {

			// Avoid a full recomputation if one of the plotting-related parameters of the modifier change.
			if(event.field() == PROPERTY_FIELD(fixRealSpaceXAxisRange) ||
					event.field() == PROPERTY_FIELD(fixRealSpaceYAxisRange) ||
					event.field() == PROPERTY_FIELD(realSpaceXAxisRangeStart) ||
					event.field() == PROPERTY_FIELD(realSpaceXAxisRangeEnd) ||
					event.field() == PROPERTY_FIELD(realSpaceYAxisRangeStart) ||
					event.field() == PROPERTY_FIELD(realSpaceYAxisRangeEnd) ||
					event.field() == PROPERTY_FIELD(fixReciprocalSpaceXAxisRange) ||
					event.field() == PROPERTY_FIELD(fixReciprocalSpaceYAxisRange) ||
					event.field() == PROPERTY_FIELD(reciprocalSpaceXAxisRangeStart) ||
					event.field() == PROPERTY_FIELD(reciprocalSpaceXAxisRangeEnd) ||
					event.field() == PROPERTY_FIELD(reciprocalSpaceYAxisRangeStart) ||
					event.field() == PROPERTY_FIELD(reciprocalSpaceYAxisRangeEnd) ||
					event.field() == PROPERTY_FIELD(normalizeRealSpace) ||
					event.field() == PROPERTY_FIELD(normalizeRealSpaceByRDF) ||
					event.field() == PROPERTY_FIELD(normalizeRealSpaceByCovariance) ||
					event.field() == PROPERTY_FIELD(normalizeReciprocalSpace) ||
					event.field() == PROPERTY_FIELD(typeOfRealSpacePlot) ||
					event.field() == PROPERTY_FIELD(typeOfReciprocalSpacePlot))
				return true;

			return Engine::modifierChanged(event);
		}

		/// Compute real and reciprocal space correlation function via FFT.
		void computeFftCorrelation();

		/// Compute real space correlation function via direction summation over neighbors.
		void computeNeighCorrelation();

		/// Compute means and covariance.
		void computeLimits();

		/// Returns the property storage that contains the input particle positions.
		const ConstPropertyPtr& positions() const { return _positions; }

		/// Returns the property storage that contains the first input particle property.
		const ConstPropertyPtr& sourceProperty1() const { return _sourceProperty1; }

		/// Returns the property storage that contains the second input particle property.
		const ConstPropertyPtr& sourceProperty2() const { return _sourceProperty2; }

		/// Returns the simulation cell data.
		const DataOORef<const SimulationCellObject>& cell() const { return _simCell; }

		/// Returns the FFT cutoff radius.
		FloatType fftGridSpacing() const { return _fftGridSpacing; }

		/// Returns the neighbor cutoff radius.
		FloatType neighCutoff() const { return _neighCutoff; }

		/// Returns the real-space correlation function.
		const PropertyPtr& realSpaceCorrelation() const { return _realSpaceCorrelation; }

		/// Returns the RDF evaluated from an FFT correlation.
		const PropertyPtr& realSpaceRDF() const { return _realSpaceRDF; }

		/// Returns the short-ranged real-space correlation function.
		const PropertyPtr& neighCorrelation() const { return _neighCorrelation; }

		/// Returns the RDF evalauted from a direct sum over neighbor shells.
		const PropertyPtr& neighRDF() const { return _neighRDF; }

		/// Returns the reciprocal-space correlation function.
		const PropertyPtr& reciprocalSpaceCorrelation() const { return _reciprocalSpaceCorrelation; }

		/// Returns the mean of the first property.
		FloatType mean1() const { return _mean1; }

		/// Returns the mean of the second property.
		FloatType mean2() const { return _mean2; }

		/// Returns the variance of the first property.
		FloatType variance1() const { return _variance1; }

		/// Returns the variance of the second property.
		FloatType variance2() const { return _variance2; }

		/// Returns the (co)variance.
		FloatType covariance() const { return _covariance; }

		void setMoments(FloatType mean1, FloatType mean2, FloatType variance1,
					    FloatType variance2, FloatType covariance) {
			_mean1 = mean1;
			_mean2 = mean2;
			_variance1 = variance1;
			_variance2 = variance2;
			_covariance = covariance;
		}

	private:

		/// Real-to-complex FFT.
		std::vector<std::complex<FloatType>> r2cFFT(int nX, int nY, int nZ, std::vector<FloatType>& rData);

		/// Complex-to-real inverse FFT
		std::vector<FloatType> c2rFFT(int nX, int nY, int nZ, std::vector<std::complex<FloatType>>& cData);

		/// Map property onto grid.
		std::vector<FloatType>  mapToSpatialGrid(const PropertyObject* property,
							  size_t propertyVectorComponent,
							  const AffineTransformation& reciprocalCell,
							  int nX, int nY, int nZ,
							  bool applyWindow);

		const size_t _vecComponent1;
		const size_t _vecComponent2;
		const FloatType _fftGridSpacing;
		const bool _applyWindow;
		const FloatType _neighCutoff;
		const AveragingDirectionType _averagingDirection;
		DataOORef<const SimulationCellObject> _simCell;
		ConstPropertyPtr _positions;
		ConstPropertyPtr _sourceProperty1;
		ConstPropertyPtr _sourceProperty2;

		PropertyPtr _realSpaceCorrelation;
		FloatType _realSpaceCorrelationRange;
		PropertyPtr _realSpaceRDF;
		PropertyPtr _neighCorrelation;
		PropertyPtr _neighRDF;
		PropertyPtr _reciprocalSpaceCorrelation;
		FloatType _reciprocalSpaceCorrelationRange;
		FloatType _mean1 = 0;
		FloatType _mean2 = 0;
		FloatType _variance1 = 0;
		FloatType _variance2 = 0;
		FloatType _covariance = 0;
	};

private:

	/// The particle property that serves as the first data source for the correlation function.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ParticlePropertyReference, sourceProperty1, setSourceProperty1);
	/// The particle property that serves as the second data source for the correlation function.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ParticlePropertyReference, sourceProperty2, setSourceProperty2);
	/// Controls the cutoff radius for the FFT grid.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, fftGridSpacing, setFFTGridSpacing);
	/// Controls if a windowing function should be applied in non-periodic directions.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, applyWindow, setApplyWindow, PROPERTY_FIELD_MEMORIZE);
	/// Controls whether the real-space correlation should be computed by direct summation.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, doComputeNeighCorrelation, setComputeNeighCorrelation, PROPERTY_FIELD_MEMORIZE);
	/// Controls the cutoff radius for the neighbor lists.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, neighCutoff, setNeighCutoff, PROPERTY_FIELD_MEMORIZE);
	/// Controls the number of bins for the neighbor part of the real-space correlation function.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, numberOfNeighBins, setNumberOfNeighBins, PROPERTY_FIELD_MEMORIZE);
	/// Controls the averaging direction.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(AveragingDirectionType, averagingDirection, setAveragingDirection, PROPERTY_FIELD_MEMORIZE);
	/// Controls the normalization of the real-space correlation function.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(NormalizationType, normalizeRealSpace, setNormalizeRealSpace, PROPERTY_FIELD_MEMORIZE);
	/// Controls the normalization by rdf of the real-space correlation function.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, normalizeRealSpaceByRDF, setNormalizeRealSpaceByRDF, PROPERTY_FIELD_MEMORIZE);
	/// Controls the normalization by covariance of the real-space correlation function.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, normalizeRealSpaceByCovariance, setNormalizeRealSpaceByCovariance, PROPERTY_FIELD_MEMORIZE);
	/// Type of real-space plot (lin-lin, log-lin or log-log)
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, typeOfRealSpacePlot, setTypeOfRealSpacePlot);
	/// Controls the whether the range of the x-axis of the plot should be fixed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, fixRealSpaceXAxisRange, setFixRealSpaceXAxisRange);
	/// Controls the start value of the x-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, realSpaceXAxisRangeStart, setRealSpaceXAxisRangeStart, PROPERTY_FIELD_MEMORIZE);
	/// Controls the end value of the x-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, realSpaceXAxisRangeEnd, setRealSpaceXAxisRangeEnd, PROPERTY_FIELD_MEMORIZE);
	/// Controls the whether the range of the y-axis of the plot should be fixed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, fixRealSpaceYAxisRange, setFixRealSpaceYAxisRange);
	/// Controls the start value of the y-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, realSpaceYAxisRangeStart, setRealSpaceYAxisRangeStart, PROPERTY_FIELD_MEMORIZE);
	/// Controls the end value of the y-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, realSpaceYAxisRangeEnd, setRealSpaceYAxisRangeEnd, PROPERTY_FIELD_MEMORIZE);
	/// Controls the normalization of the reciprocal-space correlation function.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(bool, normalizeReciprocalSpace, setNormalizeReciprocalSpace, PROPERTY_FIELD_MEMORIZE);
	/// Type of reciprocal-space plot (lin-lin, log-lin or log-log)
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, typeOfReciprocalSpacePlot, setTypeOfReciprocalSpacePlot);
	/// Controls the whether the range of the x-axis of the plot should be fixed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, fixReciprocalSpaceXAxisRange, setFixReciprocalSpaceXAxisRange);
	/// Controls the start value of the x-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, reciprocalSpaceXAxisRangeStart, setReciprocalSpaceXAxisRangeStart, PROPERTY_FIELD_MEMORIZE);
	/// Controls the end value of the x-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, reciprocalSpaceXAxisRangeEnd, setReciprocalSpaceXAxisRangeEnd, PROPERTY_FIELD_MEMORIZE);
	/// Controls the whether the range of the y-axis of the plot should be fixed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, fixReciprocalSpaceYAxisRange, setFixReciprocalSpaceYAxisRange);
	/// Controls the start value of the y-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, reciprocalSpaceYAxisRangeStart, setReciprocalSpaceYAxisRangeStart);
	/// Controls the end value of the y-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, reciprocalSpaceYAxisRangeEnd, setReciprocalSpaceYAxisRangeEnd);
};

}	// End of namespace
