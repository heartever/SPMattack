We lauched the attack on unmodified libgcrypt with the help of graphene-SGX. So  you may need to compile graphene-SGX successfully to reproduce the attack.
For example, you need to put SPM/libgcrypt into the folder: graphene/Libos/shim/test and follow the instructions of graphene-sgx: https://github.com/oscarlab/graphene

Here is something that may help.
----------
1. The victim source file is  SPMattack/libgcrypt-1.7.6/tests/eddsatest.c. 
1. Generating sigstruct and the token (https://github.com/oscarlab/graphene/issues/32)
../../../../Pal/src/host/Linux-SGX/signer/pal-sgx-sign -libpal ../../../../Pal/src/libpal-enclave.so -key ../../../../Pal/src/host/Linux-SGX/signer/enclave-key.pem --output eddsatest.manifest.sgx -exec tests/eddsatest -manifest manifest
../../../../Pal/src/host/Linux-SGX/signer/pal-sgx-get-token -output eddsatest.token -sig eddsatest.sig
1. HOW TO RUN AN APPLICATION IN GRAPHENE: https://github.com/oscarlab/graphene

Note that with different compilers or configurations the target addresses may change, so some changes to the addresses need to be made to the SPM attacks code (please refer to the paper to know which pages are used). 

Then you could load the kernel module, run the victim program and unload the kernel module.
Use "dmesg" to find the output of the kernel module.Usange of graphene-sgx:
