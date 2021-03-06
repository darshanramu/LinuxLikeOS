/******************************************************************************/
/* Important Spring 2015 CSCI 402 usage information:                          */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#pragma once

struct acpi_header {
        uint32_t ah_sign;
        uint32_t ah_size;
        uint8_t ah_rev;
        uint8_t ah_checksum;
        uint8_t ah_oemid[6];
        uint8_t ah_tableid[8];
        uint32_t ah_oemrev;
        uint32_t ah_creatorid;
        uint32_t ah_creatorrev;
};

void acpi_init();
void *acpi_table(uint32_t signature, int index);
