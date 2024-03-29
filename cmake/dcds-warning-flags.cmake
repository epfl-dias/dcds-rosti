function(dcds_target_enable_default_warnings target)
  set(scope PRIVATE)

  if (${ARGC} GREATER 1)
    set(scope ${vargc} ${ARGV1})
  endif ()

  target_compile_options(${target} BEFORE ${scope}
    # Turn on maximum code compliance and all the warnings
    -pedantic
    -Weverything
    # Set as error returning a C++ object from a C-linkage function
    -Wreturn-type-c-linkage
    -Werror=return-type-c-linkage
    # Turn warnings into errors
    -Werror

    -Wno-assign-enum
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-cast-align
    -Wno-cast-qual
    -Wno-conversion
    -Wno-covered-switch-default
    -Wno-global-constructors
    -Wno-error=c++20-compat
    -Wno-exit-time-destructors
    # these are just annoying while actively writing code
    -Wno-padded
    -Wno-error=unused-variable
    -Wno-error=unused-function
    -Wno-error=unused-parameter
    -Wno-error=unused-but-set-variable
    -Wno-weak-vtables

    # Following are generated by tbb (used for rw_mutex)
    -Wno-reserved-macro-identifier
    -Wno-unused-private-field
    -Wno-undef
    -Wno-reserved-identifier
    -Wno-comma

    # Following are generated by google benchmark
    -Wno-shift-sign-overflow
    -Wno-zero-as-null-pointer-constant
    )
endfunction()