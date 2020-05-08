# SPDX-License-Identifier: Apache-2.0
# zephyr_compile_options(-fdiagnostics-color=always)

macro(toolchain_cc_cpp_base_flags dest_list_name)
  list(APPEND ${dest_list_name} "-fdiagnostics-color=always")
endmacro()