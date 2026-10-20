# Native env only (extra_scripts = pre:...).
#
# On macOS the SDK's <locale.h> (pulled in by <vector>) includes <_locale.h>,
# which resolves to the project's include/_locale.h when include/ is on the -I
# path. Project headers are always included with quotes, so hand include/ to the
# compiler as -iquote instead: quoted includes still find it, the SDK's
# angle-bracket include no longer does.

import os

Import("env")

include_dir = os.path.abspath(env.subst("$PROJECT_INCLUDE_DIR"))


def iquote_include_dir(env, node):
    cpppath = [
        p for p in env.get("CPPPATH", [])
        if os.path.abspath(env.subst(str(p))) != include_dir
    ]
    return env.Object(
        node,
        CPPPATH=cpppath,
        CCFLAGS=env.get("CCFLAGS", []) + ["-iquote", include_dir],
    )


env.AddBuildMiddleware(iquote_include_dir)
