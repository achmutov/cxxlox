alias b := build
build PRESET="debug":
    cmake --preset {{ PRESET }} --log-level=VERBOSE
    cmake --build --preset {{ PRESET }} -v

run SAMPLE="sample" PRESET="debug": (build PRESET)
    ./build/{{ PRESET }}/lox {{ justfile_dir() }}/examples/{{ SAMPLE }}.lox

prompt PRESET="debug":
    ./build/{{ PRESET }}/lox
