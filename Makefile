PREFIX := kss
LIBS := -lcurl -lz -lyaml
Darwin-x86_64_LIBS := -lldap

include BuildSystem/common.mk
