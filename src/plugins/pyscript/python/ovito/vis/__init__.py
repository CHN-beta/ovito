"""
This module contains classes related to data visualization and rendering.

**Rendering:**

  * :py:class:`RenderSettings`
  * :py:class:`Viewport`
  * :py:class:`ViewportConfiguration`

**Render engines:**

  * :py:class:`OpenGLRenderer`
  * :py:class:`TachyonRenderer`

**Data visualization:**

  * :py:class:`Display` (base class of all display classes below)
  * :py:class:`BondsDisplay`
  * :py:class:`ParticleDisplay`
  * :py:class:`SimulationCellDisplay`
  * :py:class:`SurfaceMeshDisplay`
  * :py:class:`VectorDisplay`

"""

# Load the native modules.
from PyScriptRendering import *
from PyScriptViewport import *

def _get_RenderSettings_custom_range(self):
    """ 
    Specifies the range of animation frames to render if :py:attr:`.range` is ``CUSTOM_INTERVAL``.
    
    :Default: ``(0,100)`` 
    """
    return (self.customRangeStart, self.customRangeEnd)
def _set_RenderSettings_custom_range(self, range):
    if len(range) != 2: raise TypeError("Tuple or list of length two expected.")
    self.customRangeStart = range[0]
    self.customRangeEnd = range[1]
RenderSettings.custom_range = property(_get_RenderSettings_custom_range, _set_RenderSettings_custom_range)

def _get_RenderSettings_size(self):
    """ 
    Specifies the resolution of the generated image or movie in pixels. 
    
    :Default: ``(640,480)`` 
    """
    return (self.outputImageWidth, self.outputImageHeight)
def _set_RenderSettings_size(self, size):
    if len(size) != 2: raise TypeError("Tuple or list of length two expected.")
    self.outputImageWidth = size[0]
    self.outputImageHeight = size[1]
RenderSettings.size = property(_get_RenderSettings_size, _set_RenderSettings_size)

def _get_RenderSettings_filename(self):
    """ 
    A string specifying the file name under which the rendered image or movie will be saved.
    
    :Default: ``None``
    """
    if self.saveToFile and self.imageFilename: return self.imageFilename
    else: return None
def _set_RenderSettings_filename(self, filename):
    if filename:
        self.imageFilename = filename
        self.saveToFile = True
    else:
        self.saveToFile = False
RenderSettings.filename = property(_get_RenderSettings_filename, _set_RenderSettings_filename)

def _Viewport_render(self, settings = None):
    """ Renders an image or movie of the viewport's view.
    
        :param settings: A render settings object, which specifies the resolution and filename of the output image. 
                         If ``None``, the global render settings are used (see :py:attr:`DataSet.render_settings <ovito.DataSet.render_settings>` attribute).
        :type settings: :py:class:`RenderSettings`
        :returns: ``True`` on success; ``False`` if operation has been canceled by the user.
    """
    if settings == None:
        settings = self.dataset.render_settings
    elif isinstance(settings, dict):
        settings = RenderSettings(settings)
    return self.dataset.renderScene(settings, self)
Viewport.render = _Viewport_render


# Implement the 'overlays' property of the Viewport class. 
def _get_Viewport_overlays(self):
    
    def ViewportOverlayList__delitem__(self, index):
        if index < 0: index += len(self)
        if index >= len(self): raise IndexError("List index is out of range.")
        self.__vp.removeOverlay(index)

    def ViewportOverlayList__setitem__(self, index, overlay):
        if index < 0: index += len(self)
        if index >= len(self): raise IndexError("List index is out of range.")
        self.__vp.removeOverlay(index)
        self.__vp.insertOverlay(index, overlay)

    def ViewportOverlayList_append(self, overlay):
        self.__vp.insertOverlay(len(self), overlay)
            
    def ViewportOverlayList_insert(self, index, overlay):
        if index < 0: index += len(self)        
        self.__vp.insertOverlay(index, overlay)

    overlay_list = self._overlays
    overlay_list.__vp = self
    type(overlay_list).__delitem__ = ViewportOverlayList__delitem__
    type(overlay_list).__setitem__ = ViewportOverlayList__setitem__
    type(overlay_list).append = ViewportOverlayList_append
    type(overlay_list).insert = ViewportOverlayList_insert
    return overlay_list
Viewport.overlays = property(_get_Viewport_overlays)

def _ViewportConfiguration__len__(self):
    return len(self.viewports)
ViewportConfiguration.__len__ = _ViewportConfiguration__len__

def _ViewportConfiguration__iter__(self):
    return self.viewports.__iter__()
ViewportConfiguration.__iter__ = _ViewportConfiguration__iter__

def _ViewportConfiguration__getitem__(self, index):
    return self.viewports[index]
ViewportConfiguration.__getitem__ = _ViewportConfiguration__getitem__
