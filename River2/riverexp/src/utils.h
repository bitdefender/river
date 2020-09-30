#include <vector>
#include <string>

//////////
//Some global utility functions for parsing, testing, etc.
/**
 * hex2int
 * take a hex string and convert it to a 32bit number (max 8 hex digits)
 */

class ConcolicExecutionResult;
class PathConstraint;

class Utils
{
public:

    //////////////
    // Some global definitions & configs

    // Enable this to append (assert X)  before any let instruction read from output file
    static const bool MANUALLY_ADD_ASSERT_PREFIX = true;
    /////////////

    static int hex2int(char *hex) 
    {
        int val = 0;
        while (*hex) {
            // get current character then increment
            uint8_t byte = *hex++; 
            // transform hex character to the 4bit equivalent number, using the ascii table indexes
            if (byte >= '0' && byte <= '9') byte = byte - '0';
            else if (byte >= 'a' && byte <='f') byte = byte - 'a' + 10;
            else if (byte >= 'A' && byte <='F') byte = byte - 'A' + 10;    
            // shift 4 to make space for new digit, and add the 4 bits of the new digit 
            val = (val << 4) | (byte & 0xF);
        }
        return val;
    }

    /* A utility function to reverse a string  */
    static void reverse_str(char str[], int length) 
    { 
        int start = 0; 
        int end = length -1; 
        while (start < end) 
        { 
            std::swap(*(str+start), *(str+end)); 
            start++; 
            end--; 
        } 
    }

    // Implementation of itoa() 
    static char* my_itoa(int num, char* str, int base) 
    { 
        int i = 0; 
        bool isNegative = false; 
    
        /* Handle 0 explicitely, otherwise empty string is printed for 0 */
        if (num == 0) 
        { 
            str[i++] = '0'; 
            str[i] = '\0'; 
            return str; 
        } 
    
        // In standard itoa(), negative numbers are handled only with  
        // base 10. Otherwise numbers are considered unsigned. 
        if (num < 0 && base == 10) 
        { 
            isNegative = true; 
            num = -num; 
        } 
    
        // Process individual digits 
        while (num != 0) 
        { 
            int rem = num % base; 
            str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0'; 
            num = num/base; 
        } 
    
        // If number is negative, append '-' 
        if (isNegative) 
            str[i++] = '-'; 
    
        str[i] = '\0'; // Append string terminator 
    
        // Reverse the string 
        reverse_str(str, i); 
    
        return str; 
    } 

    static std::vector<std::string> splitStringBySeparator(const std::string& s, char seperator)
    {
        std::vector<std::string> output;

        std::string::size_type prev_pos = 0, pos = 0;

        while((pos = s.find(seperator, pos)) != std::string::npos)
        {
            std::string substring( s.substr(prev_pos, pos-prev_pos) );

            output.push_back(substring);

            prev_pos = ++pos;
        }

        if (prev_pos < pos)
            output.push_back(s.substr(prev_pos, pos-prev_pos)); // Last word

        return output;
    }

    static bool is_number(const std::string& s)
    {
        if (s.empty())
            return false;

        for (const char c : s)
        {
            if (!std::isdigit(c))
            return false;
        }		

        return true;
    }

    // Merges all individual unique variables from all tests in a single data structure
    static void mergeAllVariables(PathConstraint& outPathConstraint);

    // Converts between the two data structures
    static void convertExecResultToPathConstraint(const ConcolicExecutionResult& inExecResults, PathConstraint& outPathConstraint);

};