/tmp/tmp.iFpaPmjroE/cmake-build-remote_dias_server/benchmarks/microbenches/counter-based-ds/counter-based-ds
; ModuleID = 'MY COOL JIT'
source_filename = "MY COOL JIT"

@counter_value = common global i32 0, align 4

declare void @check()

define i32 @update_and_read_fn(i32* %0) {
entry:
  %counter_value_read_variable = alloca i32, align 4
  %1 = load i32, i32* %0, align 4
  store i32 %1, i32* %counter_value_read_variable, align 4
  %counter_value_write_variable = alloca i32, align 4
  store i32 7, i32* %counter_value_write_variable, align 4
  %2 = load i32, i32* %counter_value_write_variable, align 4
  store i32 %2, i32* %0, align 4
  %counter_value_read_variable1 = alloca i32, align 4
  %3 = load i32, i32* %0, align 4
  store i32 %3, i32* %counter_value_read_variable1, align 4
  %dummy_var1 = alloca i32, align 4
  store i32 10, i32* %dummy_var1, align 4
  %dummy_var2 = alloca i32, align 4
  store i32 11, i32* %dummy_var2, align 4
  %4 = load i32, i32* %dummy_var1, align 4
  %5 = load i32, i32* %dummy_var2, align 4
  %6 = add i32 %4, %5
  store i32 %6, i32* %dummy_var1, align 4
  %dummy_var3 = alloca i32, align 4
  store i32 100, i32* %dummy_var3, align 4
  %7 = load i32, i32* %dummy_var1, align 4
  %8 = load i32, i32* %dummy_var2, align 4
  %9 = add i32 %7, %8
  store i32 %9, i32* %dummy_var3, align 4
  %ifCond = icmp ne i32* %dummy_var1, %dummy_var2
  br i1 %ifCond, label %then, label %else

then:                                             ; preds = %entry
  %10 = load i32, i32* %dummy_var1, align 4
  %11 = load i32, i32* %dummy_var2, align 4
  %12 = add i32 %10, %11
  store i32 %12, i32* %dummy_var1, align 4
  %13 = load i32, i32* %dummy_var1, align 4
  %14 = load i32, i32* %dummy_var2, align 4
  %15 = add i32 %13, %14
  store i32 %15, i32* %dummy_var3, align 4
  br label %ifcont

else:                                             ; preds = %entry
  %16 = load i32, i32* %dummy_var1, align 4
  %17 = load i32, i32* %dummy_var2, align 4
  %18 = add i32 %16, %17
  store i32 %18, i32* %dummy_var3, align 4
  br label %ifcont

ifcont:                                           ; preds = %else, %then
  %19 = load i32, i32* %counter_value_read_variable1, align 4
  ret i32 %19
}

define i32 @Counter() {
entry:
  call void @check()
  store i32 2, i32* @counter_value, align 4
  ret i32 0
}
Hello, World!!
WARNING: All log messages before absl::InitializeLog() is called are written to STDERR
I0000 00:00:1691489537.765241   31234 codegen.hpp:357] 0

I0000 00:00:1691489537.765417   31234 codegen.hpp:363] 7


Process finished with exit code 0
