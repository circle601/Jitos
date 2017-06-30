
template<class First = char, class ...Rest>
void CC(First first, Rest... rest) {
	WorkPlace[iOut++] = first;
	CC(rest...);
}

template<class First = char>
void CC(First first) {
	WorkPlace[iOut++] = first;
	if (iOut > MaxWorkPlaceSise) Error("workplace overflow");
}

