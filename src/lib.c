int clamp(int num, int min, int max) {
	if (num < min) {
		return min;
	}
	if (num > max) {
		return max;
	}
	return num;
}

float clampf(float num, float min, float max) {
	if (num < min) {
		return min;
	}
	if (num > max) {
		return max;
	}
	return num;
}
