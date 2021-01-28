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
master_doc = 'Manual'

# General information about the project.
project = 'OVITO'
copyright = '2021 OVITO GmbH'

# The version info for the project you're documenting, acts as replacement for
# |version| and |release|, also used in various other places throughout the
# built documents.

import ovito
# The short X.Y version.
version = "{}.{}".format(ovito.version[0], ovito.version[1])
# The full version, including alpha/beta/rc tags.
release = ovito.version_string

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

# -- Options for HTML output ----------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
html_theme = 'rtd'

# Theme options are theme-specific and customize the look and feel of a theme
# further.  For a list of options available for each theme, see the
# documentation.
#html_theme_options = {}

# Add any paths that contain custom themes here, relative to this directory.
html_theme_path = ["."]

# The name for this set of Sphinx documents.  If None, it defaults to
# "<project> v<release> documentation".
#html_title = None

# A shorter title for the navigation bar.  Default is the same as html_title.
#html_short_title = None

# The name of an image file (relative to this directory) to place at the top
# of the sidebar.
#html_logo = "ovito_logo.png"

# The name of an image file (within the static path) to use as favicon of the
# docs.  This file should be a Windows icon file (.ico) being 16x16 or 32x32
# pixels large.
html_favicon = "ovito.ico"

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

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

# Output file base name for HTML help builder.
htmlhelp_basename = 'OVITOdoc'

def process_docstring(app, what, name, obj, options, lines):
    # Filter out lines that contain the keyword "SIGNATURE:"
    # These lines allow the C++ code to specify a custom function signature string for the Python documentation.
    lines[:] = [line for line in lines if not line.strip().startswith('SIGNATURE:')]

def process_signature(app, what, name, obj, options, signature, return_annotation):
    # Look for keyword "SIGNATURE:" in the docstring.
    # This allows the C++ code to specify a custom function signature string for the Python documentation.
    if obj.__doc__:
        for line in obj.__doc__.splitlines():
            if line.strip().startswith("SIGNATURE:"):
                return (line[len("SIGNATURE:"):].strip(), return_annotation)
    return (signature, return_annotation)

def skip_member(app, what, name, obj, skip, options):
    # This will skip class aliases and exclude them from the documentation:
    if what == "module" and getattr(obj, "__name__", name) != name:
        return True

    # Skip objects whose docstring contains the special keyword 'AUTODOC_SKIP_MEMBER'.
    # This is mainly needed, because pybind11 automatically generates docstrings for enum types,
    # and there is no other way to suppress the including of these enums in the documentation.
    if "AUTODOC_SKIP_MEMBER" in str(getattr(obj, "__doc__", "")):
        return True

    return None
'''
import docutils
from sphinx.util.nodes import split_explicit_title

def ovitoman_role(typ, rawtext, text, lineno, inliner, options={}, content=[]):
    """Role for linking to the OVITO user manual."""
    env = inliner.document.settings.env
    text = docutils.utils.unescape(text)
    has_explicit, title, target = split_explicit_title(text)
    if '#' in target:
        target, anchor = target.split('#')
        anchor = '#' + anchor
    else:
        anchor = ''
    url = env.config.ovito_user_manual_url + '/' + target + env.config.html_file_suffix + anchor
    ref = docutils.nodes.reference(rawtext, title, refuri=url)
    return [ref], []
'''
def setup(app):
    app.connect('autodoc-process-docstring', process_docstring)
    app.connect('autodoc-process-signature', process_signature)
    app.connect('autodoc-skip-member', skip_member)
    #app.add_role('ovitoman', ovitoman_role)
    #app.add_config_value('ovito_user_manual_url', '.', 'html')
   