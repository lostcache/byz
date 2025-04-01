sudo dtrace -c './byz' -o out.stacks -n 'profile-997 /execname == "byz"/ { @[ustack(100)] = count(); }'
stackcollapse.pl out.stacks | flamegraph.pl > flamegraph.svg
