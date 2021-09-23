////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 OVITO GmbH, Germany
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

#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/properties/AffineTransformationParameterUI.h>

namespace Ovito {

IMPLEMENT_OVITO_CLASS(AffineTransformationParameterUI);

/******************************************************************************
* Constructor for a Qt property.
******************************************************************************/
AffineTransformationParameterUI::AffineTransformationParameterUI(PropertiesEditor* parentEditor, const char* propertyName, size_t _row, size_t _column, const QString& labelText, const QMetaObject* parameterUnitType)
	: FloatParameterUI(parentEditor, propertyName, labelText, parameterUnitType), row(_row), column(_column)
{
	OVITO_ASSERT_MSG(row >= 0 && row < 3, "AffineTransformationParameterUI constructor", "The row must be in the range 0-2.");
	OVITO_ASSERT_MSG(column >= 0 && column < 4, "AffineTransformationParameterUI constructor", "The column must be in the range 0-3.");
}

/******************************************************************************
* Constructor for a PropertyField property.
******************************************************************************/
AffineTransformationParameterUI::AffineTransformationParameterUI(PropertiesEditor* parentEditor, const PropertyFieldDescriptor& propField, size_t _row, size_t _column)
	: FloatParameterUI(parentEditor, propField), row(_row), column(_column)
{
	OVITO_ASSERT_MSG(row >= 0 && row < 3, "AffineTransformationParameterUI constructor", "The row must be in the range 0-2.");
	OVITO_ASSERT_MSG(column >= 0 && column < 4, "AffineTransformationParameterUI constructor", "The column must be in the range 0-3.");
}

/******************************************************************************
* Takes the value entered by the user and stores it in the parameter object
* this parameter UI is bound to.
******************************************************************************/
void AffineTransformationParameterUI::updatePropertyValue()
{
	if(editObject() && spinner()) {
		try {
			if(isQtPropertyUI()) {
				QVariant currentValue = editObject()->property(propertyName());
				if(currentValue.canConvert<AffineTransformation>()) {
					AffineTransformation val = currentValue.value<AffineTransformation>();
					val(row, column) = spinner()->floatValue();
					currentValue.setValue(val);
				}
				if(!editObject()->setProperty(propertyName(), currentValue)) {
					OVITO_ASSERT_MSG(false, "AffineTransformationParameterUI::updatePropertyValue()", qPrintable(QString("The value of property %1 of object class %2 could not be set.").arg(QString(propertyName()), editObject()->metaObject()->className())));
				}
			}
			else if(isPropertyFieldUI()) {
				QVariant currentValue = editObject()->getPropertyFieldValue(*propertyField());
				if(currentValue.canConvert<AffineTransformation>()) {
					AffineTransformation val = currentValue.value<AffineTransformation>();
					val(row, column) = spinner()->floatValue();
					currentValue.setValue(val);
				}
				editor()->changePropertyFieldValue(*propertyField(), currentValue);
			}
			Q_EMIT valueEntered();
		}
		catch(const Exception& ex) {
			ex.reportError();
		}
	}
}

/******************************************************************************
* This method updates the displayed value of the parameter UI.
******************************************************************************/
void AffineTransformationParameterUI::updateUI()
{
	if(editObject() && spinner() && !spinner()->isDragging()) {
		QVariant val;
		if(isQtPropertyUI()) {
			val = editObject()->property(propertyName());
			OVITO_ASSERT_MSG(val.isValid() && (val.canConvert<AffineTransformation>()), "AffineTransformationParameterUI::updateUI()", QString("The object class %1 does not define a property with the name %2 that can be cast to an AffineTransformation type.").arg(editObject()->metaObject()->className(), QString(propertyName())).toLocal8Bit().constData());
			if(!val.isValid() || !(val.canConvert<AffineTransformation>())) {
				editObject()->throwException(tr("The object class %1 does not define a property with the name %2 that can be cast to an AffineTransformation type.").arg(editObject()->metaObject()->className(), QString(propertyName())));
			}
		}
		else if(isPropertyFieldUI()) {
			val = editObject()->getPropertyFieldValue(*propertyField());
			OVITO_ASSERT(val.isValid() && (val.canConvert<AffineTransformation>()));
		}
		else return;

		if(val.canConvert<AffineTransformation>())
			spinner()->setFloatValue(val.value<AffineTransformation>()(row, column));
	}
}

}	// End of namespace

