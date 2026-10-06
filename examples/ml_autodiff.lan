main() i32:
    a = Tensor.ones2(2, 2)
    b = Tensor.ones2(2, 2)
    tape = Grad.create()
    Grad.watch(tape, a)
    c = Grad.mul(tape, a, b)
    y = Grad.tanh(tape, c)
    loss = Grad.sum(tape, y)
    Grad.backward(tape, loss)
    g = Grad.grad(tape, a)
    total: f64 = g.sum()
    print(total)

    g.free()
    loss.free()
    y.free()
    c.free()
    tape.free()
    b.free()
    a.free()

    if total < 1.67 || total > 1.69:
        return 1
    return 0
