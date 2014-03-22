
#include "app_hdr.h"

__attribute__ ((section("app_hdr")))
const app_hdr_t _app_hdr = {
    .magic = "BBMT-APP",
    .major_version = MAJOR_VERSION,
    .minor_version = MINOR_VERSION,
    .patch_version = PATCH_VERSION
};
