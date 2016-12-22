"""
This module contains data structures and container classes that are used by OVITO's modification pipeline system.

**Data collection:**

  * :py:class:`DataCollection` (a container for data objects and attributes)

**Data objects:**

  * :py:class:`DataObject` (base of all data object types)
  * :py:class:`Bonds`
  * :py:class:`BondProperty`
  * :py:class:`BondTypeProperty`
  * :py:class:`DislocationNetwork`
  * :py:class:`ParticleProperty`
  * :py:class:`ParticleTypeProperty`
  * :py:class:`SimulationCell`
  * :py:class:`SurfaceMesh`
  * :py:class:`TrajectoryLineGenerator`

**Auxiliary classes:**

  * :py:class:`ParticleType`
  * :py:class:`BondType`
  * :py:class:`CutoffNeighborFinder`
  * :py:class:`NearestNeighborFinder`
  * :py:class:`DislocationSegment`

"""

import numpy as np

try:
    # Python 3.x
    import collections.abc as collections
except ImportError:
    # Python 2.x
    import collections

# Load the native module.
from PyScriptScene import DataCollection
from PyScriptScene import DataObject
from PyScriptApp import CloneHelper

# Give the DataCollection class a dict-like interface.
DataCollection.__len__ = lambda self: len(self.objects)

def _DataCollection__iter__(self):
    for o in self.objects:
        if hasattr(o, "_data_key"):
            yield o._data_key
        else:
            yield o.object_title
DataCollection.__iter__ = _DataCollection__iter__

def _DataCollection__getitem__(self, key):
    for o in self.objects:
        if hasattr(o, "_data_key"):
            if o._data_key == key:
                return o
        else:
            if o.object_title == key:
                return o
    raise KeyError("DataCollection does not contain object key '%s'." % key)
DataCollection.__getitem__ = _DataCollection__getitem__

def _DataCollection__getattr__(self, name):
    for o in self.objects:
        if hasattr(o, "_data_attribute_name"):
            if o._data_attribute_name == name:
                return o
    raise AttributeError("DataCollection does not have an attribute named '%s'." % name)
DataCollection.__getattr__ = _DataCollection__getattr__

def _DataCollection__str__(self):
    return "DataCollection(" + str(list(self.keys())) + ")"
DataCollection.__str__ = _DataCollection__str__

# Mix in base class collections.Mapping:
DataCollection.__bases__ = DataCollection.__bases__ + (collections.Mapping, )

# Implement the 'attributes' property of the DataCollection class.
def _DataCollection_attributes(self):
    """
    A dictionary object with key/value pairs that have been loaded from an input file
    or were produced by modifiers in the data pipeline.
    
    Attributes are integer, float, or string values that are processed or generated by the 
    modification pipeline. They represent global information or scalar quantities. This is in contrast
    to more complex *data objects*, which are also stored in the :py:class:`!DataCollection` such as particle properties or bonds.
    
    Attribute names (dictionary keys) are simple strings such as ``"Timestep"`` or
    ``"ConstructSurfaceMesh.surface_area"``.

    **Attributes loaded from input files**

    The ``Timestep`` attribute is loaded from LAMMPS dump files and other simulation file formats
    that store the simulation timestep. This kind of information read from an input file can be retrieved from 
    the attributes dictionary of the :py:attr:`~ovito.ObjectNode.source` data collection as follows::

        >>> node = ovito.dataset.selected_node
        >>> node.source.attributes['Timestep']
        140000
        
    Note that the attributes dictionary will contain all key/value pairs parsed from the 
    comment line in the header of *extended XYZ* files.
    
    **Attributes computed by modifiers**
    
    Analysis modifiers like the :py:class:`~ovito.modifiers.CommonNeighborAnalysisModifier` or
    the :py:class:`~ovito.modifiers.ClusterAnalysisModifier` output scalar computation results
    as attributes. The class documentation of each modifier lists the names of the attributes it
    produces.
    
    For example, the number of clusters generated by the :py:class:`~ovito.modifiers.ClusterAnalysisModifier`
    can be queried as follows::
    
        node.modifiers.append(ClusterAnalysisModifier(cutoff = 3.1))
        node.compute()
        nclusters = node.output.attributes["ClusterAnalysis.cluster_count"]
        
    **Exporting attributes to a text file**
    
    The :py:func:`ovito.io.export_file` function allows writing selected attributes to a text
    file, possibly as functions of time::
    
        export_file(node, "data.txt", "txt", 
            columns = ["Timestep", "ClusterAnalysis.cluster_count"], 
            multiple_frames = True)
            
    **User-defined attributes**
    
    The :py:class:`~ovito.modifiers.PythonScriptModifier` makes it possible for you to define your own
    attributes that are dynamically computed (on the basis of other information)::
    
        node.modifiers.append(CommonNeighborAnalysisModifier())
        
        def compute_fcc_fraction(frame, input, output):
            n_fcc = input.attributes['CommonNeighborAnalysis.counts.FCC']
            output.attributes['fcc_fraction'] = n_fcc / input.number_of_particles
        
        node.modifiers.append(PythonScriptModifier(function = compute_fcc_fraction))
        node.compute()
        print(node.output.attributes['fcc_fraction'])
        
    In this example the :py:class:`~ovito.modifiers.CommonNeighborAnalysisModifier` outputs the computed
    attribute ``CommonNeighborAnalysis.counts.FCC``, which is the absolute number of atoms that 
    form an FCC lattice. To compute the fraction of FCC atoms, we need to divide by the total number of 
    atoms in the system. To this end, a :py:class:`~ovito.modifiers.PythonScriptModifier` is defined and
    inserted into the pipeline following the :py:class:`~ovito.modifiers.CommonNeighborAnalysisModifier`.
    The user-defined modifier function generates a new attribute named ``fcc_fraction``. Finally, 
    after the pipeline has been evaluated, the value of the user-defined attribute can 
    be accessed as shown or exported to a text file.
    
    """
    
    # Helper class used to implement the 'attributes' property of the DataCollection class.
    class _AttributesView(collections.MutableMapping):
        
        def __init__(self, data_collection):
            self._data_collection = data_collection
            
        def __len__(self):
            return len(self._data_collection.attribute_names)
        
        def __getitem__(self, key):
            if not isinstance(key, str):
                raise TypeError("Attribute key is not a string.")
            v = self._data_collection.get_attribute(key)
            if v is not None:
                return v
            raise KeyError("DataCollection contains no attribute named '%s'." % key)

        def __setitem__(self, key, value):
            if not isinstance(key, str):
                raise TypeError("Attribute key is not a string.")
            self._data_collection.set_attribute(key, value)

        def __delitem__(self, key):
            if not isinstance(key, str):
                raise TypeError("Attribute key is not a string.")
            v = self._data_collection.get_attribute(key)
            if v is None:
                raise KeyError("DataCollection contains no attribute named '%s'." % key)
            self._data_collection.set_attribute(key, None)

        def __iter__(self):
            for aname in self._data_collection.attribute_names:
                yield aname

        def __repr__(self):
            return repr(dict(self))
     
    return _AttributesView(self)
DataCollection.attributes = property(_DataCollection_attributes)

# Implement the DataCollection.copy_if_needed() method.
def _DataCollection_copy_if_needed(self, obj):
    """
        Makes a copy of a data object if it was created upstream in the data pipeline.

        Typically, this method is used in the user-defined implementation of a :py:class:`~ovito.modifiers.PythonScriptModifier` that
        participates in OVITO's data pipeline system. The user-defined modifier function receives a collection with
        input data objects from the system. However, directly modifying these input
        objects is not allowed because they are owned by the upstream part of the data pipeline.
        This is where this method comes into play: It makes a copy of a data object and replaces
        it with its copy in the modifier's output. The modifier can then go ahead and modify the copy as needed,
        because it is now exclusively owned by the modifier.

        The method first checks if *obj*, which must be a data object from this data collection, is
        owned by anybody else. If yes, it creates an exact copy of *obj* and replaces the original
        in this data collection with the copy. Now the copy is an independent object, which is referenced
        by nobody except this data collection. Thus, the modifier function is now free to modify the contents
        of the data object.

        Note that the :py:meth:`!copy_if_needed` method should always be called on the *output* data collection
        of the modifier.

        :param DataObject obj: The object in the output data collection to be copied.
        :return: An exact copy of *obj* if *obj* is owned by someone else. Otherwise the original instance is returned.
    """
    assert(isinstance(obj, DataObject))
    # The object to be modified must be in this data collection.
    if not obj in self.values():
        raise ValueError("DataCollection.copy_if_needed() must be called with an object that is part of this data collection.")
    # Check if object is owned by someone else.
    # This is indicated by the fact that the object has more than one dependent (which would be this data collection).
    if obj.num_dependents > 1:
        # Make a copy of the object so it can be safely modified.
        clone = CloneHelper().clone(obj, False)
        self.replace(obj, clone)
        return clone
    return obj
DataCollection.copy_if_needed = _DataCollection_copy_if_needed

def _DataCollection_to_ase_atoms(self):
    """
    Constructs and returns an `ASE Atoms object <https://wiki.fysik.dtu.dk/ase/ase/atoms.html>`_ from the particles
    stored in this :py:class:`!DataCollection`.

    .. note::

       Calling this method raises an ImportError if ASE (`Atomistic Simulation Environment <https://wiki.fysik.dtu.dk/ase/>`_) is not available. Note that the built-in
       Python interpreter shipping with OVITO does *not* contain the ASE module.
       It is therefore recommended to build OVITO from source (as explained in the user manual),
       which will allow you to use all modules installed in the system's Python interpreter.

    :return: A new `ASE Atoms object <https://wiki.fysik.dtu.dk/ase/ase/atoms.html>`_ that contains the
             contains the converted particle data from this :py:class:`!DataCollection`.
    """

    from ase.atoms import Atoms

    # Extract basic dat: pbc, cell, positions, particle types
    pbc = self.cell.pbc
    cell_matrix = np.array(self.cell.matrix)
    cell, origin = cell_matrix[:, :3].T, cell_matrix[:, 3]
    info = {'cell_origin': origin }
    positions = np.array(self.position)
    type_names = dict([(t.id, t.name) for t in
                       self.particle_type.type_list])
    symbols = [type_names[id] for id in np.array(self.particle_type)]

    # construct ase.Atoms object
    atoms = Atoms(symbols,
                  positions,
                  cell=cell,
                  pbc=pbc,
                  info=info)

    # Convert any other particle properties to additional arrays
    for name, prop in self.items():
        if name in ['Simulation cell',
                    'Position',
                    'Particle Type']:
            continue
        if not isinstance(prop, ParticleProperty):
            continue
        prop_name = prop.name
        i = 1
        while prop_name in atoms.arrays:
            prop_name = '{0}_{1}'.format(prop.name, i)
            i += 1
        atoms.new_array(prop_name, prop.array)

    return atoms

DataCollection.to_ase_atoms = _DataCollection_to_ase_atoms


def _DataCollection_create_from_ase_atoms(cls, atoms):
    """
    Converts an `ASE Atoms object <https://wiki.fysik.dtu.dk/ase/ase/atoms.html>`_ to a :py:class:`!DataCollection`.

    .. note::

       The built-in Python interpreter shipping with OVITO does *not* contain the ASE module (`Atomistic Simulation Environment <https://wiki.fysik.dtu.dk/ase/>`_).
       It is therefore recommended to build OVITO from source (as explained in the user manual),
       which will allow you to use all modules installed in the system's Python interpreter.

    :param atoms: The `ASE Atoms object <https://wiki.fysik.dtu.dk/ase/ase/atoms.html>`_ to be converted.
    :return: A new :py:class:`!DataCollection` instance containing the converted data from the ASE object.
    """
    data = cls()

    # Set the unit cell and origin (if specified in atoms.info)
    cell = SimulationCell()
    matrix = np.zeros((3,4))
    matrix[:, :3] = atoms.get_cell().T
    matrix[:, 3]  = atoms.info.get('cell_origin',
                                   [0., 0., 0.])
    cell.matrix = matrix
    cell.pbc = [bool(p) for p in atoms.get_pbc()]
    data.add(cell)

    # Add ParticleProperty from atomic positions
    num_particles = len(atoms)
    position = ParticleProperty.create(ParticleProperty.Type.Position,
                                       num_particles)
    position.marray[...] = atoms.get_positions()
    data.add(position)

    # Set particle types from chemical symbols
    types = ParticleProperty.create(ParticleProperty.Type.ParticleType,
                                    num_particles)
    symbols = atoms.get_chemical_symbols()
    type_list = list(set(symbols))
    for i, sym in enumerate(type_list):
        types.type_list.append(ParticleType(id=i+1, name=sym))
    types.marray[:] = [ type_list.index(sym)+1 for sym in symbols ]
    data.add(types)

    # Check for computed properties - forces, energies, stresses
    calc = atoms.get_calculator()
    if calc is not None:
        for name, ptype in [('forces', ParticleProperty.Type.Force),
                            ('energies', ParticleProperty.Type.PotentialEnergy),
                            ('stresses', ParticleProperty.Type.StressTensor),
                            ('charges', ParticleProperty.Type.Charge)]:
            try:
                array = calc.get_property(name,
                                          atoms,
                                          allow_calculation=False)
                if array is None:
                    continue
            except NotImplementedError:
                continue

            # Create a corresponding OVITO standard property.
            prop = ParticleProperty.create(ptype, num_particles)
            prop.marray[...] = array
            data.add(prop)

    # Create extra properties in DataCollection
    for name, array in atoms.arrays.items():
        if name in ['positions', 'numbers']:
            continue
        if array.dtype.kind == 'i':
            typ = 'int'
        elif array.dtype.kind == 'f':
            typ = 'float'
        else:
            continue
        num_particles = array.shape[0]
        num_components = 1
        if len(array.shape) == 2:
            num_components = array.shape[1]
        prop = ParticleProperty.create_user(name,
                                            typ,
                                            num_particles,
                                            num_components)
        prop.marray[...] = array
        data.add(prop)

    return data

DataCollection.create_from_ase_atoms = classmethod(_DataCollection_create_from_ase_atoms)
