Adapted from the UFUG2106 Group 2 research archive by Jincan LI and Zeyun DU.
[Round 5.1 frozen source](https://github.com/Jincan-LI-HUB/Truth-Table-to-Circuit-Discrete-Math-Project/tree/171229f0be/Stage1_Freeze_Round5.1_Verified/source_snapshot/src).

The integrated solver uses the research BDD and two-level cover mapper, typed graph rebalancing and area recovery, algebraic rewriting and polarity mapping. Its scheduler supplies the input table, influence estimates and deadline from the Stage 2 solver. The standalone CLI and legacy fallback are excluded.

The Stage 2 finishing pass uses Jincan Li's [R10.0 four-leaf cut mapper](https://github.com/Jincan-LI-HUB/Truth-Table-to-Circuit-Discrete-Math-Project/blob/4838e50d52408087639c3d3816b10761f18f6ccb/UFUG2106_Round10_0_Delivery/submission/r84_cut.hpp)
with the repeat-SOP settings from the same release. It runs once on this solver's
incumbent. The Stage 2 CLI supplies the budget and checks whole-circuit metrics.
