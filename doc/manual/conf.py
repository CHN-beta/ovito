# -*- coding: utf-8 -*-
#
# OVITO User Manual configuration file.
#
# This file is execfile()d with the current directory set to its
# containing dir.
#
# Note that not all possible configuration values are present in this
# autogenerated file.
#
# All configuration values have a default; values that are commented out
# serve to show the default.

import sys
import os

# Use the Read-The-Docs theme.
import sphinx_rtd_theme

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#sys.path.insert(0, os.path.abspath('.'))

# -- General configuration ------------------------------------------------

# If your documentation needs a minimal Sphinx version, state it here.
needs_sphinx = '2.0'

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.intersphinx',
    'sphinx_rtd_theme'
]

autodoc_default_options = {
    'members': True,
    'imported-members': True
}

autodoc_inherit_docstrings = False

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# The suffix of source filenames.
source_suffix = '.rst'

# The encoding of source files.
#source_encoding = 'utf-8-sig'

# The master toctree document.
master_doc = 'index'

# General information about the project.
project = 'OVITO User Manual'

# The version info for the project you're documenting, acts as replacement for
# |version| and |release|, also used in various other places throughout the
# built documents.

# The language for content autogenerated by Sphinx. Refer to documentation
# for a list of supported languages.
#language = None

# There are two options for replacing |today|: either, you set today to some
# non-false value, then it is used:
#today = ''
# Else, today_fmt is used as the format for a strftime call.
#today_fmt = '%B %d, %Y'

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
exclude_patterns = []

# The reST default role (used for this markup: `text`) to use for all
# documents.
#default_role = None

# If true, '()' will be appended to :func: etc. cross-reference text.
#add_function_parentheses = True

# If true, the current module name will be prepended to all description
# unit titles (such as .. function::).
#add_module_names = True

# If true, sectionauthor and moduleauthor directives will be shown in the
# output. They are ignored by default.
#show_authors = False

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = 'sphinx'

# A list of ignored prefixes for module index sorting.
#modindex_common_prefix = []

# A string of reStructuredText that will be included at the beginning of every source file that is read.
rst_prolog = """
.. highlight:: console

.. role:: ovito-pro-tag-role(raw)
   :format: html
   
.. |ovito-pro| replace:: :ovito-pro-tag-role:`<a class="ovito-pro-tag" href="https://www.ovito.org/about/ovito-pro/" data-tooltip="This program feature is only available in the Pro edition of OVITO. Click to learn more." data-tooltip-position="right">pro</a>`
"""

# If true, keep warnings as "system message" paragraphs in the built documents.
keep_warnings = False

# If true, Sphinx will warn about all references where the target cannot be found.
nitpicky = True
nitpick_ignore = [
    ('py:class', 'bool'),
    ('py:class', 'int'),
    ('py:class', 'tuple'),
    ('py:class', 'str'),
    ('py:class', 'float')
]

# Locations and names of other projects that should be linked to in this documentation.
if os.getenv("OVITO_PYDOC_INTERSPHINX_LOCATION"):
    intersphinx_mapping = { 'pydoc': ('python/', os.getenv("OVITO_PYDOC_INTERSPHINX_LOCATION")) }
else:
    intersphinx_mapping = { 'pydoc': ('https://ovito.org/docs/current/python/', None) }

# -- Options for HTML output ----------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
html_theme = 'sphinx_rtd_theme'

# Theme options are theme-specific and customize the look and feel of a theme
# further.  For a list of options available for each theme, see the
# documentation.
html_theme_options = {
    # Only display the logo image, do not display the project name at the top of the sidebar
    'logo_only': True
}

# Add any paths that contain custom themes here, relative to this directory.
#html_theme_path = ["."]

# The name for this set of Sphinx documents.  If None, it defaults to
# "<project> v<release> documentation".
#html_title = None

# A shorter title for the navigation bar.  Default is the same as html_title.
#html_short_title = None

# The name of an image file (relative to this directory) to place at the top
# of the sidebar.
html_logo = "_static/ovito_logo.png"

# The name of an image file (within the static path) to use as favicon of the
# docs.  This file should be a Windows icon file (.ico) being 16x16 or 32x32
# pixels large.
html_favicon = "ovito.ico"

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

# These paths are either relative to html_static_path
# or fully qualified paths (eg. https://...)
html_css_files = ['style.css']

# Add any extra paths that contain custom files (such as robots.txt or
# .htaccess) here, relative to this directory. These files are copied
# directly to the root of the documentation.
#html_extra_path = []

# If not '', a 'Last updated on:' timestamp is inserted at every page bottom,
# using the given strftime format.
#html_last_updated_fmt = '%b %d, %Y'

# If true, SmartyPants will be used to convert quotes and dashes to
# typographically correct entities.
#html_use_smartypants = True

# Custom sidebar templates, maps document names to template names.
html_sidebars = {
   '**': ['globaltoc.html', 'sourcelink.html', 'searchbox.html']
}

# Additional templates that should be rendered to pages, maps page names to
# template names.
#html_additional_pages = {}

# If false, no module index is generated.
#html_domain_indices = True

# If false, no index is generated.
#html_use_index = True

# If true, the index is split into individual pages for each letter.
#html_split_index = False

# If true, links to the reST sources are added to the pages.
html_show_sourcelink = False

# If true, the reST sources are included in the HTML build as _sources/name.
html_copy_source = False

# If true, "Created using Sphinx" is shown in the HTML footer. Default is True.
html_show_sphinx = False

# If true, "(C) Copyright ..." is shown in the HTML footer. Default is True.
#html_show_copyright = True

# If true, an OpenSearch description file will be output, and all pages will
# contain a <link> tag referring to it.  The value of this option must be the
# base URL from which the finished HTML is served.
#html_use_opensearch = ''

# This is the file name suffix for HTML files (e.g. ".xhtml").
html_file_suffix = ".html"

# Configure optional spelling extension if present.
# See https://sphinxcontrib-spelling.readthedocs.io/en/latest/
try:
    import enchant
    import importlib.util
    if importlib.util.find_spec("sphinxcontrib.spelling"):
        extensions.append('sphinxcontrib.spelling')

        # String specifying the language, as understood by PyEnchant and enchant.
        #spelling_lang='en_US'

        # String specifying a file containing a list of words known to be spelled correctly but that do not appear in the language 
        # dictionary selected by 'spelling_lang'. The file should contain one word per line. 
        #spelling_word_list_filename='spelling_wordlist.txt'

        # Boolean controlling whether suggestions for misspelled words are printed.    
        #spelling_show_suggestions = False

        # Boolean controlling whether the contents of the line containing each misspelled word is printed, for more context about the location of each word.
        #spelling_show_whole_line = True

        # Boolean controlling whether a misspelling is emitted as a sphinx warning or as an info message.
        spelling_warning = True

        # A list of glob-style patterns that should be ignored when checking spelling. They are matched against the 
        # source file names relative to the source directory, using slashes as directory separators on all platforms.
        spelling_exclude_patterns=['licenses/*']
except:
    pass
