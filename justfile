alias b := build
build PRESET="debug":
    cmake --preset {{ PRESET }}
    cmake --build --preset {{ PRESET }}

run SAMPLE="./examples/sample.lox" PRESET="debug": (build PRESET)
    ./build/{{ PRESET }}/lox {{ justfile_dir() }}/examples/{{ SAMPLE }}.lox
