"""Sphinx configuration for libplatform's C API documentation."""

from pathlib import Path

project = "libplatform"
copyright = "2026, Zachary Fogg"
author = "Zachary Fogg"
release = "master"

extensions = [
    "myst_parser",
    "hawkmoth",
    "hawkmoth.ext.javadoc",
]

source_suffix = {".md": "markdown", ".rst": "restructuredtext"}
master_doc = "index"
exclude_patterns = ["_build"]

# Hawkmoth resolves autodoc paths from this directory. Keeping the include
# tree as the source root means the API page names the same paths users include.
hawkmoth_root = str(Path(__file__).resolve().parents[1] / "include")
hawkmoth_compiler = "clang"
hawkmoth_autoconf = []
hawkmoth_clang = ["-I", str(Path(__file__).resolve().parents[1] / "include")]

html_theme = "alabaster"
html_title = "libplatform"
html_short_title = "libplatform"
html_static_path = ["_static"]
html_css_files = ["theme.css"]
html_theme_options = {
    "description": "Cross-platform C foundations without the #ifdef maintenance chore.",
    "github_user": "zfogg",
    "github_repo": "libplatform",
    "github_button": True,
    "show_related": False,
    "show_powered_by": False,
}

nitpicky = False
