#ifndef DRIVES_H
#define DRIVES_H

#include <mem.h>
#include <partition/partition.h>
#include <stdint.h>
#include <sys/types.h>

struct kdrive_t;

typedef ssize_t (*kdrive_read_sectors)(struct kdrive_t *drive, size_t lba,
                                       size_t count, uint8_t *buf);
typedef ssize_t (*kdrive_write_sectors)(struct kdrive_t *drive, size_t lba,
                                        size_t count, const uint8_t *buf);

// drive struct representation
struct kdrive_t {
  char name[64];
  char sysname[16];

  uint32_t sector_size;
  uint32_t userdata1;
  uint32_t userdata2;

  struct partition_table_t partitions;
  kdrive_read_sectors read;
  kdrive_write_sectors write;
};
//register a drive
void register_kdrive(struct kdrive_t *drive);
//get a certain drive 
struct kdrive_t *get_kdrive(int i);
//initializes drives by calling ata
void drives_init();

#endif
