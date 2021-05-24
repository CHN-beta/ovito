.. _file_formats.input:
  
Input file formats
------------------

OVITO can read the following file formats:

.. list-table:: 
  :widths: 20 55 25 
  :header-rows: 1

  * - Format name
    - Description 
    - Data type(s)
  
  * - LAMMPS data
    - File format used by the `LAMMPS <http://lammps.sandia.gov/doc/read_data.html>`__ molecular dynamics code.
    - :ref:`particles <scene_objects.particles>`, :ref:`bonds <scene_objects.bonds>`, angles, dihedrals, impropers
  
  * - LAMMPS dump
    - File format used by the `LAMMPS <http://lammps.sandia.gov>`__  LAMMPS molecular dynamics code. OVITO supports both text-based and binary dump files.
    - :ref:`particles <scene_objects.particles>`

  * - LAMMPS dump local
    - File format written by the `dump local <https://lammps.sandia.gov/doc/dump.html>`__ command of LAMMPS. 
      OVITO's :ref:`particles.modifiers.load_trajectory` modifier can read varying bond topology and 
      per-bond quantities from such files generated in reactive molecular dynamics simulations.  
    - :ref:`bonds <scene_objects.bonds>`

  * - ReaxFF bonds
    - File format written by the LAMMPS `fix reax/c/bonds <https://lammps.sandia.gov/doc/fix_reaxc_bonds.html>`__ command and the original ReaxFF code of Adri van Duin. 
      OVITO's :ref:`particles.modifiers.load_trajectory` modifier can read the bond topology, bond order and 
      atomic charges dumped during ReaxFF molecular dynamics simulations.  
    - :ref:`bonds <scene_objects.bonds>`

  * - XYZ 
    - Simple column-based text format for particle data, which is documented `here <http://en.wikipedia.org/wiki/XYZ_file_format>`__. 
      OVITO can read the `extended XYZ format <https://web.archive.org/web/20190811094343/https://libatoms.github.io/QUIP/io.html#extendedxyz>`__, 
      which supports arbitrary sets of particle data columns, and can store additional information such as the simulation cell geometry and boundary conditions. 
    - :ref:`particles <scene_objects.particles>`
  
  * - POSCAR / XDATCAR / CHGCAR 
    -  File formats used by the *ab initio* simulation package `VASP <http://www.vasp.at/>`__.
       OVITO can import atomistic configurations and also charge density fields from CHGCAR files.     
    - :ref:`particles <scene_objects.particles>`, :ref:`voxel grids <scene_objects.voxel_grid>`       
             
  * - Gromacs GRO            
    - Coordinate file format used by the `GROMACS <http://www.gromacs.org/>`__ simulation code.
    - :ref:`particles <scene_objects.particles>`             
  
  * - Gromacs XTC
    - Trajectory file format used by the `GROMACS <http://www.gromacs.org/>`__ simulation code.
    - :ref:`particles <scene_objects.particles>`

  * - CFG 
    - File format used by the `AtomEye <http://li.mit.edu/Archive/Graphics/A/>`__ visualization program.           
    - :ref:`particles <scene_objects.particles>`

  * - NetCDF 
    - Binary format for molecular dynamics data following the `AMBER format convention <http://ambermd.org/netcdf/nctraj.pdf>`__. NetCDF files are produced by 
      the LAMMPS `dump netcdf <http://lammps.sandia.gov/doc/dump_netcdf.html>`__ command.  
    - :ref:`particles <scene_objects.particles>`
             
  * - CIF          
    - `Crystallographic Information File <https://www.iucr.org/resources/cif>`__ format as specified by the 
      International Union of Crystallography (IUCr). Parser supports only small-molecule crystal structures.  
    - :ref:`particles <scene_objects.particles>`

  * - PDB 
    - Protein Data Bank (PDB) files.  
    - :ref:`particles <scene_objects.particles>`
           
  * - PDBx/mmCIF 
    - The `PDBx/mmCIF <http://mmcif.wwpdb.org>`__ format stores 
      macromolecular structures and is used by the Worldwide Protein Data Bank.   
    - :ref:`particles <scene_objects.particles>`
             
  * - Quantum Espresso 
    - Input data format used by the `Quantum Espresso <https://www.quantum-espresso.org/>`__ electronic-structure calculation code.  
    - :ref:`particles <scene_objects.particles>`
             
  * - FHI-aims 
    - Geometry and log-file formats used by the *ab initio* simulation package `FHI-aims <https://aimsclub.fhi-berlin.mpg.de/index.php>`__.  
    - :ref:`particles <scene_objects.particles>`
  
  * - GSD/HOOMD 
    - Binary format for molecular dynamics data used by the `HOOMD-blue <https://glotzerlab.engin.umich.edu/hoomd-blue/>`__ code. 
      See `GSD (General Simulation Data) format <https://gsd.readthedocs.io>`__.  
    - :ref:`particles <scene_objects.particles>`          

  * - CASTEP       
    - File format used by the `CASTEP <http://www.castep.org>`__ *ab initio* code. OVITO can read the |castep formats|_.

        .. |castep formats| replace:: :file:`.cell`, :file:`.md` and :file:`.geom` formats
        .. _castep formats: http://www.tcm.phy.cam.ac.uk/castep/documentation/WebHelp/content/modules/castep/expcastepfileformats.htm
    - :ref:`particles <scene_objects.particles>`
                        
  * - XSF 
    - File format used by the `XCrySDen <http://www.xcrysden.org/doc/XSF.html>`__ program.  
    - :ref:`particles <scene_objects.particles>`, :ref:`voxel grids <scene_objects.voxel_grid>`

  * - Cube 
    - File format used by the *Gaussian* simulation package. Specifications of the format can be found
      `here <https://h5cube-spec.readthedocs.io/en/latest/cubeformat.html>`__ and `here <http://paulbourke.net/dataformats/cube/>`__.  
    - :ref:`particles <scene_objects.particles>`, :ref:`voxel grids <scene_objects.voxel_grid>`             
             
  * - IMD 
    - File format used by the molecular dynamics code `IMD <http://imd.itap.physik.uni-stuttgart.de/>`__.  
    - :ref:`particles <scene_objects.particles>`             

  * - DL_POLY 
    - File format used by the molecular simulation package `DL_POLY <https://www.scd.stfc.ac.uk/Pages/DL_POLY.aspx>`__.  
    - :ref:`particles <scene_objects.particles>`
                        
  * - GALAMOST 
    - XML-based file format used by the *GALAMOST* molecular dynamics code.  
    - :ref:`particles <scene_objects.particles>`, :ref:`bonds <scene_objects.bonds>` 

  * - VTK (legacy format) 
    - File format used by the *Visualization Toolkit* (VTK) and the software *ParaView*. The format is described `here <http://www.vtk.org/VTK/img/file-formats.pdf>`__. 
      The file reader currently supports only ASCII-based files containing PolyData and UnstructuredGrid data with triangular cells.  
    - :ref:`triangle meshes <scene_objects.triangle_mesh>` 
             
  * - VTI (VTK ImageData) 
    - XML-based file format used by the *Visualization Toolkit* (VTK) and the software *ParaView*. The format is described `here <http://www.vtk.org/VTK/img/file-formats.pdf>`__. 
      The file reader currently supports only a subset of the full format specification and is geared towards files written by the `Aspherix <https://www.aspherix-dem.com/>`__ simulation code.  
    - :ref:`voxel grids <scene_objects.voxel_grid>`
               
  * - VTP (VTK PolyData) 
    - XML-based file format used by the *Visualization Toolkit* (VTK) and the software *ParaView*. The format is described `here <http://www.vtk.org/VTK/img/file-formats.pdf>`__. 
      The file reader currently supports only a subset of the full format specification and is geared towards mesh geometry and particle data files written by the `Aspherix <https://www.aspherix-dem.com/>`__ simulation code.
      VTK PolyData blocks consisting of triangle strips or polygons are imported as surface meshes by OVITO. PolyData consisting of vertices only are imported as a set of particles.  
    - :ref:`surface meshes <scene_objects.surface_mesh>`, :ref:`particles <scene_objects.particles>`
               
  * - VTM (VTK MultiBlock) 
    - XML-based file format used by the *Visualization Toolkit* (VTK) and the software *ParaView*. VTK multiblock data files are meta-files that point to a list of VTK XML files,
      which will all be loaded by OVITO as a single data collection.  
    - any 
                
  * - PVD (ParaView data file) 
    - XML-based file format used by the software *ParaView*, which describes a trajectory formed by a sequence of individual data files.
      The file format is described `here <https://www.paraview.org/Wiki/ParaView/Data_formats#PVD_File_Format>`__.  
    - any 
                   
  * - OBJ 
    - Common text-based format for storing triangle mesh geometry (see `here <https://en.wikipedia.org/wiki/Wavefront_.obj_file>`__).  
    - :ref:`triangle meshes <scene_objects.triangle_mesh>`
             
  * - STL 
    - Another popular format for storing triangle mesh geometry (see `here <https://en.wikipedia.org/wiki/STL_(file_format)>`__). Note that OVITO supports only STL files in ASCII format.  
    - :ref:`triangle meshes <scene_objects.triangle_mesh>`
             
  * - PARCAS 
    - Binary file format written by the MD code *Parcas* developed in K. Nordlund's group at University of Helsinki.  
    - :ref:`particles <scene_objects.particles>`
             
  * - ParaDiS 
    - File format of the `ParaDiS <http://paradis.stanford.edu>`__ discrete dislocation dynamics code.  
    - :ref:`dislocation lines <scene_objects.dislocations>`         
             
  * - oxDNA 
    - Configuration/topology file format used by the `oxDNA <https://dna.physics.ox.ac.uk/>`__ simulation code for coarse-grained DNA models.  
    - :ref:`particles <scene_objects.particles>`, :ref:`bonds <scene_objects.bonds>` 
