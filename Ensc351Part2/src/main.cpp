#include "posixThread.hpp"

using namespace std;
using namespace pthreadSupport;

int Ensc351Part2(); // should we include a header file?

int main()
{
    try {       // ... realtime policy.
        // program should be starting off at realtime priority 98 or 99
        if (getSchedPrio() < 98)
            cout << "**** If you are debugging, debugger is not running at a high priority. ****\n" <<
                    " **** This could cause problems with debugging.  Consider debugging\n" <<
                    " **** with the proper debug launch configuration ****" << endl;

        setSchedPrio(60); // drop priority down somewhat.  FIFO?
        return Ensc351Part2();
    }
    catch (system_error& error) {
        cout << "Error: " << error.code() << " - " << error.what() << '\n';
        cout << "Have you launched process with a realtime scheduling policy?" << endl;
        return error.code().value();
    }
    catch (...) { throw; }
}

