#include <stdio.h>

#include <stringutil.h>
#include <Stream.h>

#include "dummy.h"

void cross_test()
{
    scrub_email_addr("abc@efg.com");
    Stream stm;
    stm.isValid();
}

