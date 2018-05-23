This is the code for our Sneaky Page Monitoring (SPM) attack published at ACM CCS 2017: [Leaky Cauldron on the Dark Land: Understanding Memory Side-Channel Hazards in SGX](https://heartever.github.io/files/leaky.pdf).
We lauched the attack on unmodified libgcrypt with the help of graphene-SGX. So you may need to compile graphene-SGX successfully to reproduce the attack.

## How to run the victim library with graphene-SGX

Put SPM/libgcrypt into the folder: graphene/Libos/shim/test and follow the instructions of [graphene-sgx](https://github.com/oscarlab/graphene). Here is something that may help.

* The victim source file is  SPMattack/libgcrypt-1.7.6/tests/eddsatest.c. 
* Compile the libcrypt-1.7.6 binary with `make`.
* Generate sigstruct and the token (https://github.com/oscarlab/graphene/issues/32). Change to the directory `graphene/LibOS/shim/test/libgcrypt-1.7.6` and run
```
../../../../Pal/src/host/Linux-SGX/signer/pal-sgx-sign -libpal ../../../../Pal/src/libpal-enclave.so -key ../../../../Pal/src/host/Linux-SGX/signer/enclave-key.pem -output eddsatest.manifest.sgx -exec tests/eddsatest -manifest manifest
../../../../Pal/src/host/Linux-SGX/signer/pal-sgx-get-token -output eddsatest.token -sig eddsatest.sig
```
* Please note that you need to pin the run to core 3 with the help of ``taskset -c 3``, as we are sending IPIs to core 3.

* HOW TO RUN AN APPLICATION IN GRAPHENE: https://github.com/oscarlab/graphene

Note that with different compilers or configurations the target addresses may change, so some changes to the addresses need to be made to the SPM attacks code (please refer to [our paper](https://heartever.github.io/files/leaky.pdf) to know which pages are used). 

## How to run the kernel module

We hooked the default page fault handler by modifying `linux/arch/x86/mm/fault.c`. Please look into the modifed `fault.c` for kernel version 4.2.8. Similar changes can be made to other kernel version. Then recompile and boot into the new kernel. 

Now you could load the kernel module, run the victim program and unload the kernel module.
Use `dmesg` to find the output of the kernel module.

## Possible Compiling Issues
* sched_setaffinity undefined! 

It's possibly due to that in some kernel versions "sched_setaffinity" is not exported.
Please modify the kernel source in "kernel/sched/core.c" and locate the definition of sched_setaffinity. Then add "EXPORT_SYMBOL(sched_setaffinity);" and recompile the kernel. So that it can be referenced in a kernel module.

* Incompatible pointer type

Please try to modify the function type definition to: ``static inline void check_accessed(void *);`` and ``static inline int clear_accessed_thread(void *data)``.
