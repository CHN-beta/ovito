# Load the native modules and other dependencies.
from ..plugins.PyScript import FileExporter, AttributeFileExporter, PipelineStatus, PipelineObject
from ..data import DataCollection, DataObject
from ..pipeline import Pipeline, StaticSource
import ovito

def export_file(data, file, format, **params):
    """ High-level function that exports data to a file.
        See the :ref:`file_output_overview` section for an overview of this topic.
    
        :param data: The object to be exported. See below for options.
        :param str file: The output file path.
        :param str format: The type of file to write. See below for options.

        **Data to export**

        Various kinds of objects are accepted as *data* parameter by the function:

           * :py:class:`~ovito.pipeline.Pipeline`: 
             Exports the data dynamically generated by a pipeline. Since pipelines can be evaluated at different animation times,
             multi-frame sequences can be produced from a single :py:class:`~ovito.pipeline.Pipeline` object.  

           * :py:class:`~ovito.data.DataCollection`:
             Exports the data contained in a data collection. Data objects in the collection that are not compatible
             with the chosen output format are ignored. Since a data collection represents a static dataset, no frame
             sequences can be written.

           * :py:class:`~ovito.data.DataObject`:
             Exports the data object as if it were the only part of a :py:class:`~ovito.data.DataCollection`.

           * ``None``:
             All pipelines that are part of the current scene (see :py:attr:`ovito.DataSet.scene_pipelines`) are
             exported. This option makes sense for scene descriptions such as the POV-Ray format.

        **Output format**

        The *format* parameter determines the type of file to write; the filename suffix is ignored.
        However, for filenames that end with ``.gz``, automatic gzip compression is activated if the selected format is text-based.
        The following *format* strings are supported:

            * ``"txt"`` -- Exporting global attributes to a text file (see below)
            * ``"lammps/dump"`` -- LAMMPS text-based dump format
            * ``"lammps/data"`` -- LAMMPS data format
            * ``"imd"`` -- IMD format
            * ``"vasp"`` -- POSCAR format
            * ``"xyz"`` -- XYZ format
            * ``"fhi-aims"`` -- FHI-aims format
            * ``"netcdf/amber"`` -- Binary format for MD data following the `AMBER format convention <http://ambermd.org/netcdf/nctraj.pdf>`__ 
            * ``"vtk/trimesh"`` -- ParaView VTK format for exporting :py:class:`~ovito.data.SurfaceMesh` objects
            * ``"vtk/disloc"`` -- ParaView VTK format for exporting :py:class:`~ovito.data.DislocationNetwork` objects
            * ``"ca"`` -- `Text-based format for storing dislocation lines <../../particles.modifiers.dislocation_analysis.html#particles.modifiers.dislocation_analysis.fileformat>`__
            * ``"povray"`` -- POV-Ray scene format

        Depending on the selected format, additional keyword arguments must be passed to :py:func:`!export_file`:
       
        **File columns**
        
        For the output formats *lammps/dump*, *xyz*, *imd* and *netcdf/amber*, you must specify the set of particle properties to export 
        using the ``columns`` keyword parameter::
        
            export_file(pipeline, "output.xyz", "xyz", columns = 
              ["Particle Identifier", "Particle Type", "Position.X", "Position.Y", "Position.Z"]
            )
            
        You can export the :ref:`standard particle properties <particle-types-list>` and any user-defined properties present 
        in the pipeline's output :py:class:`~ovito.data.DataCollection`. 
        For vector properties, the component name must be appended to the base name as 
        demonstrated above for the ``Position`` property. 
        
        **Exporting several simulation frames**
        
        By default, only the current animation frame (frame 0 by default) is exported by the function.
        To export a different frame, pass the ``frame`` keyword parameter to the :py:func:`!export_file` function. 
        Alternatively, you can export all frames of the current animation sequence at once by passing ``multiple_frames=True``. Refined
        control of the exported frame sequence is available through the keyword arguments ``start_frame``, ``end_frame``, and ``every_nth_frame``.
        
        The *lammps/dump* and *xyz* file formats can store multiple frames in a single output file. For other formats, or
        if you intentionally want to generate one file per frame, you must pass a wildcard filename to :py:func:`!export_file`.
        This filename must contain exactly one ``*`` character as in the following example, which will be replaced with the
        animation frame number::

            export_file(pipeline, "output.*.dump", "lammps/dump", multiple_frames=True)
            
        The above call is equivalent to the following ``for``-loop::
        
            for i in range(pipeline.source.num_frames):
                export_file(pipeline, "output.%i.dump" % i, "lammps/dump", frame=i)

        **Floating-point formatting precision**

        For text-based file formats, you can set the desired formatting precision for floating-point numbers using the
        ``precision`` keyword parameter. The default output precision is 10 digits, the maximum is 17.
       
        **LAMMPS atom style**
        
        When writing files in the *lammps/data* format, the LAMMPS atom style "atomic" is used by default. If you want to create 
        a data file that uses a different atom style, specify it with the ``atom_style`` keyword parameter::
        
            export_file(pipeline, "output.data", "lammps/data", atom_tyle="bond")
        
        The following `LAMMPS atom styles <http://lammps.sandia.gov/doc/atom_style.html>`_ are currently supported by OVITO:
        ``angle``, ``atomic``, ``bond``, ``charge``, ``dipole``, ``full``, ``molecular``, ``sphere``.
        
        **Global attributes**
        
        The *txt* file format allows you to export global quantities computed by the data pipeline to a text file. 
        For example, to write out the number of FCC atoms identified by a :py:class:`~ovito.modifiers.CommonNeighborAnalysisModifier`
        as a function of simulation time, one would use the following::
        
            export_file(pipeline, "data.txt", "txt", 
                columns=["Timestep", "CommonNeighborAnalysis.counts.FCC"], 
                multiple_frames=True)
            
        See the documentation of the individual analysis modifiers to find out which global quantities they 
        compute. You can also determine at runtime which :py:attr:`~ovito.data.DataCollection.attributes` are available 
        in the output data collection of a :py:class:`~ovito.pipeline.Pipeline`::
        
            print(pipeline.compute().attributes)

    """

    # Determine the animation frame to be exported.    
    if 'frame' in params:
        frame = int(params['frame'])
        time = ovito.dataset.anim.frame_to_time(frame)
        params['multiple_frames'] = True
        params['start_frame'] = frame
        params['end_frame'] = frame
        del params['frame']
    else:
        frame = ovito.dataset.anim.current_frame   
        time = ovito.dataset.anim.time 

    # Look up the exporter class for the selected format.
    if not format in export_file._formatTable:
        raise RuntimeError("Unknown output file format: %s" % format)
    
    # Create an instance of the exporter class.
    exporter = export_file._formatTable[format](params)
    
    # Pass function parameters to exporter object.
    exporter.output_filename = file
    
    # Detect wildcard filename.
    if '*' in file:
        exporter.wildcard_filename = file
        exporter.use_wildcard_filename = True
    
    # Exporting to a file is a long-running operation, which is not permitted during viewport rendering or pipeline evaluation.
    # In these situations, the following function call will raise an exception.
    ovito.dataset.request_long_operation()
    
    # Pass data to be exported to the exporter:
    if isinstance(data, Pipeline):
        # Evaluate pipeline once to raise an exception if the evaluation fails.
        # That is because the exporter will silently ignore error status codes.
        data.compute(frame)
        exporter.set_pipeline(data)
    elif isinstance(data, PipelineObject):
        exporter.set_pipeline(Pipeline(source = data))
    elif isinstance(data, DataCollection):
        static_source = StaticSource()
        static_source.assign(data)
        exporter.set_pipeline(Pipeline(source = static_source))
    elif isinstance(data, DataObject):
        data_collection = DataCollection()
        data_collection.objects.append(data)
        static_source = StaticSource()
        static_source.assign(data_collection)
        exporter.set_pipeline(Pipeline(source = static_source))
    elif data is None:
        exporter.select_standard_output_data()
    else:
        raise TypeError("Invalid data. Cannot export this kind of Python object: {}".format(data))

    # Export data.
    if not exporter.export_nodes(ovito.task_manager):
        raise RuntimeError("Operation has been canceled by the user.")

# This is the table of export formats used by the export_file() function
# to look up the right exporter class for a file format.
# Plugins can register their exporter class by inserting a new entry in this dictionary.
export_file._formatTable = {}
export_file._formatTable["txt"] = AttributeFileExporter
