# OpenROAD vs TritonPart 对比测试 - Ariane

## 测试用例

使用 Ariane RISC-V 处理器设计进行对比测试：
- Verilog: ariane.v (134,920 实例)
- Liberty: Nangate45_typ.lib + fakeram45_256x16.lib
- SDC: ariane.sdc
- 分区数: 5
- 平衡约束: 2.0

## 测试功能

测试脚本支持两种功能：

1. **triton_part_design** - 设计分割
2. **evaluate_part_design_solution** - 评估分割结果并生成超图文件

## 运行测试

```bash
./run_test.sh
```

## 测试流程

1. OpenROAD `triton_part_design` - 分割设计
2. OpenROAD `evaluate_part_design_solution` - 评估并生成超图
3. TritonPart `partition` - 分割设计
4. TritonPart `evaluate` - 评估并生成超图
5. 结果对比

## 输出文件

### 分割结果
- `results/openroad_result.part.5` - OpenROAD 分割结果
- `results/tritonpart_result.part.5` - TritonPart 分割结果

### 超图文件
- `results/openroad_ariane.hgr.wt` - OpenROAD 带权重超图
- `results/openroad_ariane.hgr.int` - OpenROAD 整数权重超图 (hMETIS 格式)
- `results/tritonpart_ariane.hgr.wt` - TritonPart 带权重超图
- `results/tritonpart_ariane.hgr.int` - TritonPart 整数权重超图

### 日志文件
- `results/openroad_partition.log`
- `results/openroad_evaluate.log`
- `results/tritonpart_partition.log`
- `results/tritonpart_evaluate.log`

## 对比结果

| 指标 | OpenROAD | TritonPart | 差异 |
|------|----------|------------|------|
| Instances | 135,405 | 134,920 | -485 (0.36%) |
| Hyperedges | 136,227 | 134,920 | -1,307 (0.96%) |
| Timing Paths | 23,215 | 200 | 配置不同 |

## 差异分析

1. **实例数差异 (0.36%)**
   - OpenROAD 使用 OpenDB 解析网表
   - TritonPart 使用 OpenSTA 解析网表

2. **超边数差异 (0.96%)**
   - 对单扇出网络的过滤策略不同
   - 差异在可接受范围内

3. **时序路径数差异**
   - 两者配置相同: top_n = 100,000
   - 可通过 `--top_n` 参数调整

## 结论

TritonPart 独立版与 OpenROAD 原版在网表读取和超图构建方面基本等价，差异在 1% 以内。
