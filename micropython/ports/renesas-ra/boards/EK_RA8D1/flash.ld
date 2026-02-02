/* 
 * RA8D1 (R7FA8D1BHECBD) 非安全域链接器脚本
 */
MEMORY
{
  /* Flash: 非安全域起始地址 0x12000000, 2MB */ [22]
  FLASH (rx) : ORIGIN = 0x02000000, LENGTH = 2048K
  /* SRAM: 非安全域用户SRAM起始 0x32000000, 896KB */ 
  RAM (rwx)  : ORIGIN = 0x32000000, LENGTH = 896K
}

/*... (SECTIONS 定义保持不变，确保 _heap_start 定义在 RAM 中)...*/