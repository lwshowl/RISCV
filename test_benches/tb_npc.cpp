#include <stdlib.h>
#include <iostream>
#include <Vnpc.h>
#include <verilated.h>
#include <Vnpc__Dpi.h>
#include <verilated_vcd_c.h>
#include <thread>
#include "instructions.h"
#include <queue>
#include "color.h"
using namespace std;

#define MAX_SIM_TIME -1
vluint64_t sim_time = 0;
vluint64_t posedge_count = 0;
Vnpc *dut = new Vnpc;
queue<uint64_t> dnpc_queue;
uint64_t dnpc_at_commit;

extern "C" uint64_t vaddr_read(uint64_t addr, int len);
extern "C" uint32_t vaddr_write(uint64_t addr, int len, uint64_t data);

static const char *reg_name[] = {
    "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

uint64_t is_committing();
void dump_decode();
void dump_fetch();
void dump_alu();
void dump_wb();
void dump_dmem();
void dump_regfile();
void pipe_pass_test(Vnpc *dut);
int sanity_check();
void dump_pc();
void dump_csr();
void update_dnpc();
extern "C" uint64_t run_once();
extern "C" void npc_reset();
extern "C" void npc_dump_registers(uint64_t *regs);
extern "C" void invalid_inst(uint64_t addr);
Color::Modifier red(Color::FG_RED);
Color::Modifier def(Color::FG_DEFAULT);

uint64_t run_once()
{
    uint64_t pc;
    while (1)
    {
        pc = is_committing();
        if (sim_time < MAX_SIM_TIME && !pc)
        {
            dut->clk ^= 1;
            dut->eval();
            sim_time++;
            // dut-clk == 1 , 刚刚有一个上升沿，所有信息被写入寄存器
            if (dut->clk == 1)
            {
                posedge_count++;
                update_dnpc();
                //dump_wb();
            }
        }
        else
            break;
    }

    if ((dut->npc__DOT__wb_branch_out && dut->npc__DOT__wb_result_out) | dut->npc__DOT__wb_instrId_out == i_jalr | dut->npc__DOT__wb_instrId_out == i_mret | dut->npc__DOT__dmem_excep_out > 0 | dut->npc__DOT__wb_instrId_out == i_ecall)
    {
        if (!dnpc_queue.empty())
        {
            dnpc_at_commit = dnpc_queue.front();
            dnpc_queue.pop();
        }
    }
    else
        dnpc_at_commit = 0;

    // commit
    dut->clk ^= 1;
    dut->eval();
    if (dut->clk == 1)
    {
        //dump_wb();
        update_dnpc();
    }
    dut->clk ^= 1;
    dut->eval();
    if (dut->clk == 1)
    {
        //dump_wb();
        update_dnpc();
    }
    return pc;
}

extern "C" uint64_t npc_dump_dnpc()
{
    return dnpc_at_commit;
}

void update_dnpc()
{
    if (dut->npc__DOT__dmem_branch_out == 1 && dut->npc__DOT__dmem_instrId_out != i_jalr && dut->npc__DOT__dmem_result_out)
    {
        // cout << "dnpc pushed at " << dut->npc__DOT__dmem_pc_out << " target " << dut->npc__DOT__dmem_pc_out + dut->npc__DOT__dmem_imm64_out << endl;
        dnpc_queue.push(dut->npc__DOT__dmem_pc_out + dut->npc__DOT__dmem_imm64_out);
        // cout << endl
        //      << "update dnpc pc= " << dut->npc__DOT__dmem_pc_out << " imm= " << dut->npc__DOT__dmem_imm64_out
        //      << endl
        //      << endl;
    }

    else if (dut->npc__DOT__dmem_instrId_out == i_jalr)
    {
        dnpc_queue.push(dut->npc__DOT__dmem_rs1val_out + dut->npc__DOT__dmem_imm64_out);
        // cout << "update dnpc pc= " << dut->npc__DOT__dmem_pc_out << " sum= " << dut->npc__DOT__dmem_rs1val_out + dut->npc__DOT__dmem_imm64_out;
    }

    else if (dut->npc__DOT__dmem_excep_out > 0)
    {
        dnpc_queue.push(dut->npc__DOT__csr__DOT__mtvec);
    }

    else if (dut->npc__DOT__wb_instrId_out == i_ecall)
    {
        dnpc_queue.push(dut->npc__DOT__csr__DOT__mtvec);
    }

    else if (dut->npc__DOT__wb_instrId_out == i_mret)
    {
        dnpc_queue.push(dut->npc__DOT__csr__DOT__mepc);
    }
}

void npc_reset()
{
    dut->rst = 1;
    dut->clk ^= 1;
    dut->eval();
    dut->clk ^= 1;
    dut->eval();
    dut->clk ^= 1;
    dut->eval();
    dut->rst = 0;
}

void npc_dump_registers(uint64_t *regs)
{
    for (int i = 0; i < 32; i++)
        regs[i] = dut->npc__DOT__registerFile__DOT__registers[i];
}

void npc_mem_read(long long raddr, long long *rdata)
{
    // std::cout << "pmem_read: " << hex << raddr;
    // cout << hex << " val: " << *rdata << endl;
    // cout << "fetch pc :" << dut->npc__DOT__pc_out << " visit addr :" << raddr << endl;
    // cout << "dmem pc :" << dut->npc__DOT__dmem_pc_out << " visit addr :" << raddr << endl;

    *rdata = vaddr_read(raddr, 8);
}

// DPI import at npc.v:231
void npc_mem_write(long long addr, long long wdata, char wmask)
{
    // std::cout << "instr: " << dec << (uint64_t)(dut->npc__DOT__dmem_instrId);
    // std::cout << " pmem_write: " << hex << addr;
    // cout << hex << " val: " << wdata << endl;
    vaddr_write(addr, wmask, wdata);
}

int sanity_check()
{
    if (dut->npc__DOT__wb_instrId_out == 63)
        if (sim_time > 10)
        {
            uint64_t pc = dut->npc__DOT__wb_pc_out;
            invalid_inst(pc);
            return 1;
        }
    return 0;
}

void dump_csr()
{
    cout << "mtvec " << hex << (uint64_t)dut->npc__DOT__csr__DOT__mtvec
         << " mcause " << hex << (uint64_t)dut->npc__DOT__csr__DOT__mcause
         << " mepc " << hex << (uint64_t)dut->npc__DOT__csr__DOT__mepc
         << " mstatus " << hex << (uint64_t)dut->npc__DOT__csr__DOT__mstatus
         << endl;
}

void dump_pc()
{
    cout << "pc out: " << hex << (uint64_t)dut->npc__DOT__pc_out
         << "pc abs_b: " << hex << (uint64_t)dut->npc__DOT__pc_abs_branch
         << "pc rel_b: " << hex << (uint64_t)dut->npc__DOT__pc_rel_branch
         << endl;
}

void dump_regfile()
{
    cout << "reg instru: " << dec << (uint64_t)dut->npc__DOT__regfile_instrId_out
         << " reg pc: " << hex << (uint64_t)dut->npc__DOT__regfile_pc_out
         << " reg rs1: " << hex << (uint64_t)dut->npc__DOT__regfile_rs1_out
         << " reg rs2: " << hex << (uint64_t)dut->npc__DOT__regfile_rs2_out
         << " reg rd: " << hex << (uint64_t)dut->npc__DOT__regfile_rd_out
         << " reg branch: " << hex << (uint64_t)dut->npc__DOT__regfile_branch_out
         << " reg regw: " << hex << (uint64_t)dut->npc__DOT__regfile_regw_out
         << " reg alubypass1: " << dec << (uint64_t)dut->npc__DOT__regfile_alubypass_rs1
         << " reg alubypass2: " << dec << (uint64_t)dut->npc__DOT__regfile_alubypass_rs2
         << " reg membypass1: " << dec << (uint64_t)dut->npc__DOT__regfile_dmembypass_rs1
         << " reg membypass2: " << dec << (uint64_t)dut->npc__DOT__regfile_dmembypass_rs2
         << " reg memorybubble: " << dec << (uint64_t)dut->npc__DOT__memory_bubble
         << endl;
}

void dump_wb()
{
    cout << "wb instru: " << dec << (uint64_t)dut->npc__DOT__wb_instrId_out
         << " wb pc: " << hex << (uint64_t)dut->npc__DOT__wb_pc_out
         << " wb rd: " << reg_name[(uint64_t)dut->npc__DOT__wb_rd_out]
         << " wb memdata: " << hex << (uint64_t)dut->npc__DOT__wb_memdata_out
         << " wb regw: " << hex << (uint64_t)dut->npc__DOT__wb_regw_out
         << " wb branch: " << dec << (uint64_t)dut->npc__DOT__wb_branch_out
         << " wb memw: " << dec << (uint64_t)dut->npc__DOT__wb_memw_out
         << " wb opcode: " << dec << (uint64_t)dut->npc__DOT__wb_opcode_out
         << " wb rs1val: " << hex << (uint64_t)dut->npc__DOT__wb_rs1val_out
         << " wb rs2val: " << hex << (uint64_t)dut->npc__DOT__wb_rs2val_out
         << " wb result: " << hex << (uint64_t)dut->npc__DOT__wb_result_out
         << " wb exception: " << hex << (uint64_t)dut->npc__DOT__wb_exception
         << " wb mcause:" << hex << (uint64_t)dut->npc__DOT__csr__DOT__mcause
         << " wb mepc:" << hex << (uint64_t)dut->npc__DOT__csr__DOT__mepc
         << " wb mtvec:" << hex << (uint64_t)dut->npc__DOT__csr__DOT__mtvec
         << endl;
}

void dump_dmem()
{
    cout << "dmem instru: " << dec << (uint64_t)dut->npc__DOT__dmem_instrId_out
         << " dmem pc: " << hex << (uint64_t)dut->npc__DOT__dmem_pc_out
         << " dmem memr: " << dec << (uint64_t)dut->npc__DOT__dmem_memr_out
         << " dmem memw: " << dec << (uint64_t)dut->npc__DOT__dmem_memw_out
         << " dmem rs2val: " << hex << (uint64_t)dut->npc__DOT__dmem_rs2val_out
         << " dmem result: " << hex << (uint64_t)dut->npc__DOT__dmem_result_out
         << " dmem branch: " << dec << (uint64_t)dut->npc__DOT__dmem_branch_out
         << endl;
}

void dump_alu()
{
    cout << "alu instru: " << dec << (uint64_t)dut->npc__DOT__alu_instrId_out
         << " alu pc: " << hex << (uint64_t)dut->npc__DOT__alu_pc_out
         << " alu rs1: " << hex << (uint64_t)dut->npc__DOT__alu_rs1val_out
         << " alu rs2: " << hex << (uint64_t)dut->npc__DOT__alu_rs2val_out
         << " alu imm: " << hex << (uint64_t)dut->npc__DOT__alu_imm64_out
         << " alu result: " << hex << (uint64_t)dut->npc__DOT__alu_result
         << " dmem imm64: " << hex << (uint64_t)dut->npc__DOT__alu_imm64_out
         << " dmem benable: " << dec << (uint64_t)dut->npc__DOT__pc_rel_branch
         << endl;
}

void dump_decode()
{
    cout << "dec instru: " << hex << (uint64_t)dut->npc__DOT__decode_instr_out
         << " dec pc: " << hex << (uint64_t)dut->npc__DOT__decode_pc_out
         << " dec id: " << dec << (uint64_t)dut->npc__DOT__instr_id
         << endl;
}

void dump_fetch()
{
    if (dut->clk == 1)
    {
        cout << " fetch pc: " << hex << (uint64_t)dut->npc__DOT__pc_out
             << " fetch instr: " << hex << (uint32_t)dut->npc__DOT__instr64
             << endl;
    }
}

uint64_t is_committing()
{
    if (dut->npc__DOT__wb_instrId_out != i_invalid && dut->npc__DOT__wb_instrId_out != i_bubble)
        return (uint64_t)dut->npc__DOT__wb_pc_out;
    else
        return 0;
}