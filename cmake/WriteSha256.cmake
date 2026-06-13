if(NOT DEFINED INPUT_FILE OR INPUT_FILE STREQUAL "")
    message(FATAL_ERROR "WriteSha256.cmake requires -DINPUT_FILE=/path/to/file")
endif()

if(NOT DEFINED OUTPUT_FILE OR OUTPUT_FILE STREQUAL "")
    message(FATAL_ERROR "WriteSha256.cmake requires -DOUTPUT_FILE=/path/to/file.sha256")
endif()

file(SHA256 "${INPUT_FILE}" digest)
get_filename_component(name "${INPUT_FILE}" NAME)
file(WRITE "${OUTPUT_FILE}" "${digest}  ${name}\n")
