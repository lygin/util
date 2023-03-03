#include "tbb/concurrent_hash_map.h"
#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"
#include "Random.h"
#include <string>

using namespace tbb;
using namespace std;


// Structure that defines hashing and comparison operations for user's type.
struct MyHashCompare {
    static size_t hash( const string& x ) {
        size_t h = 0;
        for( const char* s = x.c_str(); *s; ++s )
            h = (h*17)^*s;
        return h;
    }
    //! True if strings are equal
    static bool equal( const string& x, const string& y ) {
        return x==y;
    }
};


// A concurrent hash table that maps strings to ints.
typedef concurrent_hash_map<string,int,MyHashCompare> StringTable;


// Function object for counting occurrences of strings.
struct Counter {
    StringTable& table;
    Counter( StringTable& table_ ) : table(table_) {}
    void operator()( const blocked_range<string*> range ) const {
        for( string* p=range.begin(); p!=range.end(); ++p ) {
            StringTable::accessor a;
            table.insert( a, *p );
            a->second += 1;
        }
    }
};

const size_t N = 1000000;
string Data[N];

int main() {
    // Construct empty table.
    StringTable table;
    RandomRng rand(0, 10);
    for(int i=0; i<N; ++i) {
        Data[i] = to_string(rand.rand());
    }
    blocked_range<string*> splitData(Data, Data+N, 1000);
    // Put occurrences into the table
    parallel_for(splitData, Counter(table) );


    // Display the occurrences
    for( StringTable::iterator i=table.begin(); i!=table.end(); ++i )
        printf("%s %d\n",i->first.c_str(),i->second);
    return 0;
}