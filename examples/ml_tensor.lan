main() i32:
    v = Tensor.onesF32(4)
    v.set1(3, 7.0)
    best: usize = v.argmax(0)
    sm = v.softmax(0)
    smSum: f64 = sm.sum()

    a = Tensor.zeros2(2, 3)
    b = Tensor.ones2(3, 2)
    a.fill(2.0)
    c = a.matmul(b)
    d = c.relu()
    dMean: f64 = d.mean()
    reshaped = c.reshape2(4, 1)
    transposed = reshaped.transpose2()
    sliced = transposed.slice(1, 1, 3, 1)

    image = Tensor.ones4(1, 1, 3, 3)
    kernel = Tensor.ones4(1, 1, 2, 2)
    conv = image.conv2d(kernel, 1, 0)
    convMean: f64 = conv.mean()

    printInt(best as i64)
    print(smSum)
    print(dMean)
    print(sliced.mean())
    print(convMean)

    conv.free()
    kernel.free()
    image.free()
    sliced.free()
    transposed.free()
    reshaped.free()
    d.free()
    c.free()
    b.free()
    a.free()
    sm.free()
    v.free()

    if best != 3:
        return 1
    if smSum < 0.999 || smSum > 1.001:
        return 2
    if dMean < 5.999 || dMean > 6.001:
        return 3
    if convMean < 3.999 || convMean > 4.001:
        return 4
    return 0
