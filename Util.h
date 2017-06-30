#pragma once

int Ilog2(int Number) {
		long result = Number;
		__asm {
			mov eax, result
			bsr eax, eax;
			mov result, eax;
		}
		if (2 << result > Number)result++;
		return result;
}





