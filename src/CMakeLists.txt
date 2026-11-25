# Archaic for the classic Mac OS (68k, System 6 and 7), with Retro68: see mkmac.
# main.c's main is archaic_main: mac.c's main starts the Toolbox, then calls it.
cmake_minimum_required(VERSION 3.10)
project(Archaic C)

add_application(Archaic
    main.c html.c http.c gif.c pic.c update.c
    mac.c nmactcp.c dnr.c archaic.r
    CREATOR "ArcH")

target_compile_options(Archaic PRIVATE -O2 -std=gnu89 -Wall -Wno-unused-parameter)
set_source_files_properties(main.c PROPERTIES COMPILE_DEFINITIONS main=archaic_main)
set_source_files_properties(dnr.c PROPERTIES COMPILE_OPTIONS "-std=gnu11")
