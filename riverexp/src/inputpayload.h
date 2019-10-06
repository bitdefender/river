#ifndef INPUT_PAYLOAD_H
#define INPUT_PAYLOAD_H

#include <string>
#include <vector>

class InputPayload
{
public:
	
	std::vector<unsigned char> input;
	int bound = -1;
	int score = 0;

	bool operator<(const InputPayload& other) const
	{
		return score < other.score;
 	}

 	void setByte(const int index, const unsigned char value)
 	{
 		// Increase the size of the string if needed
 		if (input.size() <= index)
 		{
 			const int diff = index - input.size() + 1;

			for (int i = 0; i < diff; i++)			
 				input.push_back(0);
 		}

 		input[index] = value;
 	}
};

#endif