We lauched the attack on unmodified libgcrypt with the help of graphene-SGX. So you may need to compile graphene-SGX successfully to reproduce the attack.

## How to run the victim library with graphene-SGX

Put SPM/libgcrypt into the folder: graphene/Libos/shim/test and follow the instructions of [graphene-sgx](https://github.com/oscarlab/graphene). Here is something that may help.

* The victim source file is  SPMattack/libgcrypt-1.7.6/tests/eddsatest.c. First compile the libcrypt-1.7.6 binary with `make`.
* Generating sigstruct and the token (https://github.com/oscarlab/graphene/issues/32).
```
../../../../Pal/src/host/Linux-SGX/signer/pal-sgx-sign -libpal ../../../../Pal/src/libpal-enclave.so -key ../../../../Pal/src/host/Linux-SGX/signer/enclave-key.pem -output eddsatest.manifest.sgx -exec tests/eddsatest -manifest manifest
../../../../Pal/src/host/Linux-SGX/signer/pal-sgx-get-token -output eddsatest.token -sig eddsatest.sig
```
* HOW TO RUN AN APPLICATION IN GRAPHENE: https://github.com/oscarlab/graphene

Note that with different compilers or configurations the target addresses may change, so some changes to the addresses need to be made to the SPM attacks code (please refer to [our paper](https://heartever.github.io/files/leaky.pdf) to know which pages are used). 

## How to run the kernel module

We hooked the default page fault handler by modifying `linux/arch/x86/mm/fault.c`. Please look into the modifed `fault.c` for kernel version 4.2.8. Similar changes can be made to other kernel version. Then recompile and boot into the new kernel. 

Now you could load the kernel module, run the victim program and unload the kernel module.
Use `dmesg` to find the output of the kernel module.
