xlz_target_tests = ['example']

foreach t : xlz_target_tests
  e = executable(t, t+'.cpp',
  dependencies : [xlz_cli_dep])
  test(t,e)
endforeach

