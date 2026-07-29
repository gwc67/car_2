❯ @Core/Src/gpio.c  我现在有一个循迹模块，是5路黑白检测，我需要你帮我写一个驱动，gpio口分别是P0，P1，P2 ，P3，P4，我需要你帮我写一些驱动，使用轮询检测状态即可，


可以定义一个 struct xvji_t 结构体 存放5路循迹模块的实时状态，我可以在上层通过 ， void xvji_copy (struct xvji_t *out) 来获取此时的实时循迹模块的状态，struct xvji_t {
    bool P0_b;
    bool P1_b;
    bool P2_b;
    bool P3_b;
    bool P4_b;
}; 分别对应gpio的 端口
