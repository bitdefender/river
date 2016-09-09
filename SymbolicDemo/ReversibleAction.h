#ifndef _REVERSIBLE_ACTION_H_
#define _REVERSIBLE_ACTION_H_

template <typename ActionItem, void actionFunc(void *, ActionItem*), int SIZE> class ReversibleAction {
private :
	struct {
		ActionItem *var;
		unsigned int step;
	} stack[SIZE];
	unsigned int top;
	unsigned int step;
	void *ctx;

public :
	ReversibleAction<ActionItem, actionFunc, SIZE>(void *context = nullptr) {
		ctx = context;
		top = 0;
		step = 0;
	}

	unsigned int GetStep() const {
		return step;
	}

	unsigned int GetTop() const {
		return top;
	}

	bool Push(ActionItem *itm) {
		if (top == SIZE) {
			return false;
		}

		stack[top].var = itm;
		stack[top].step = step;
		top++;
		return true;
	}

	void Forward() {
		step++;
	}

	void Backward() {
		step--;
		while ((top > 0) && (stack[top - 1].step > step)) {
			top--;
			actionFunc(ctx, stack[top].var);
		}
	}
};

#endif
