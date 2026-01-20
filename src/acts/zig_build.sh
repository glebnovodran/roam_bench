#!/bin/sh

OUT_DIR=${OUT_DIR:-out}

_OBJCOPY_=llvm-objcopy
_OBJDUMP_=llvm-objdump
_READELF_=llvm-readelf

mkdir -p $OUT_DIR


zig build-exe -target riscv32-freestanding -mcpu=baseline_rv32-c -mcpu=generic_rv32+m+d -ODebug roam.zig -femit-bin=$OUT_DIR/roam.elf -T roam.ld $*

$_OBJCOPY_ -O binary $OUT_DIR/roam.elf $OUT_DIR/roam.bin
$_OBJDUMP_ -d $OUT_DIR/roam.elf > $OUT_DIR/roam.txt
$_READELF_ -s -S --wide $OUT_DIR/roam.elf | tail -n +5 > $OUT_DIR/roam.info

cat $OUT_DIR/roam.info | awk '$4 == "FUNC" {print $2 " " $3 " " $8}' > $OUT_DIR/roam.func
cat $OUT_DIR/roam.info | awk '$3 == ".text" {print "$code " $5}' > $OUT_DIR/roam.code
cat $OUT_DIR/roam.info | awk '$3 == ".data" {print "$data " $5}' > $OUT_DIR/roam.data
cat $OUT_DIR/roam.info | awk '$3 == ".sdata" {print "$sdata " $5}' > $OUT_DIR/roam.sdata
cat $OUT_DIR/roam.info | awk '$8 == "__global_pointer$" {print "$gp " $2}' > $OUT_DIR/roam.gp

CODE_SIZE=`ls -l $OUT_DIR/roam.bin | awk '{print $5}'`
NUM_FUNC=`wc -l $OUT_DIR/roam.func | awk '{print $1}'`

HEAD_FILE=$OUT_DIR/roam.head
echo 'MINION 1' > $HEAD_FILE
cat $OUT_DIR/roam.code >> $HEAD_FILE
cat $OUT_DIR/roam.data >> $HEAD_FILE
cat $OUT_DIR/roam.sdata >> $HEAD_FILE
cat $OUT_DIR/roam.gp >> $HEAD_FILE
echo '$funcs' $NUM_FUNC >> $HEAD_FILE
cat $OUT_DIR/roam.func >> $HEAD_FILE
echo '$bin' $CODE_SIZE >> $HEAD_FILE

cat $HEAD_FILE $OUT_DIR/roam.bin > $OUT_DIR/roam.minion
