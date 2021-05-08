[all_opts,use_libigl_static_library] = gptoolbox_mexopts('Static',false);

mex('-v','-O',all_opts{:},'-I../include','matryoshka.cpp');

