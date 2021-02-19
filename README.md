# Language
A concept of a language compiler generator based of json instruction files

Requires a local compilation of intel <a href="https://github.com/intel/hyperscan">hyperscan</a>*


* Note: Until fixed, remember to <a href="https://github.com/intel/hyperscan/issues/292#issuecomment-762635447">Fix hyperscan</a>

Results: (timings in the end)
<code>
if(var1>10):
Start scan:
	Match for pattern 377 : if\s*\(\s*(not\s)?\s*([A-Za-z][\w\d]*?)(\s*>\s*(not\s)?\s*\d+\s*)+\s*\)\s*:

(Not working yet, known bug)
if  (  var1  > 10  and 3<40):
Start scan:

(Not working yet, known bug)
if  (  var1  > 10  and var2 == var3 and 3<40  )  :
Start scan:
	Match for pattern 1 : ([A-Za-z][\w\d]*?)\s*=\s*.+
	Match for pattern 8 : (\w+)\s+([A-Za-z][\w\d]*?)\s*=\s*.+

if var1>10:
Start scan:
	Match for pattern 289 : if\s+(not\s)?\s*([A-Za-z][\w\d]*?)(\s*>\s*(not\s)?\s*\d+\s*)+\s*:

(Not working yet, known bug)
if    var1  > 10  and 3<40:
Start scan:

(Not working yet, known bug)
if    var1  > 10  and var2 == var3 and 3<40   :
Start scan:
	Match for pattern 1 : ([A-Za-z][\w\d]*?)\s*=\s*.+
	Match for pattern 8 : (\w+)\s+([A-Za-z][\w\d]*?)\s*=\s*.+

while(var1>10):
Start scan:
	Match for pattern 553 : while\s*\(\s*(not\s)?\s*([A-Za-z][\w\d]*?)(\s*>\s*(not\s)?\s*\d+\s*)+\s*\)\s*:

(Not working yet, known bug)
while  (  var1  > 10  and 3<40):
Start scan:

(Not working yet, known bug)
while  (  var1  > 10  and var2 == var3 and 3<40  )   :
Start scan:
	Match for pattern 1 : ([A-Za-z][\w\d]*?)\s*=\s*.+
	Match for pattern 8 : (\w+)\s+([A-Za-z][\w\d]*?)\s*=\s*.+

while var1>10:
Start scan:
	Match for pattern 465 : while\s+(not\s)?\s*([A-Za-z][\w\d]*?)(\s*>\s*(not\s)?\s*\d+\s*)+\s*:

(Not working yet, known bug)
while    var1  > 10  and 3<40:
Start scan:

(Not working yet, known bug)
while   var1  > 10  and var2 == var3   and    3<40    :
Start scan:
	Match for pattern 1 : ([A-Za-z][\w\d]*?)\s*=\s*.+
	Match for pattern 8 : (\w+)\s+([A-Za-z][\w\d]*?)\s*=\s*.+

a    =     10
Start scan:
	Match for pattern 1 : ([A-Za-z][\w\d]*?)\s*=\s*.+

a=19
Start scan:
	Match for pattern 1 : ([A-Za-z][\w\d]*?)\s*=\s*.+

int   a   =  10
Start scan:
	*Match for pattern 2 : (int)\s+([A-Za-z][\w\d]*?)\s*=\s*.+
	Match for pattern 1 : ([A-Za-z][\w\d]*?)\s*=\s*.+
	Match for pattern 8 : (\w+)\s+([A-Za-z][\w\d]*?)\s*=\s*.+

int a=19
Start scan:
	*Match for pattern 2 : (int)\s+([A-Za-z][\w\d]*?)\s*=\s*.+
	Match for pattern 1 : ([A-Za-z][\w\d]*?)\s*=\s*.+
	Match for pattern 8 : (\w+)\s+([A-Za-z][\w\d]*?)\s*=\s*.+

const    int    a   = 123
Start scan:
	Match for pattern 34 : (const\s+)+(\w+)\s+([A-Za-z][\w\d]*?)\s*=\s*.+
	*Match for pattern 10 : (const\s+)+(int)\s+([A-Za-z][\w\d]*?)\s*=\s*.+
	Match for pattern 2 : (int)\s+([A-Za-z][\w\d]*?)\s*=\s*.+

const int a=124
Start scan:
	Match for pattern 34 : (const\s+)+(\w+)\s+([A-Za-z][\w\d]?)\s*=\s*.+
	*Match for pattern 10 : (const\s+)+(int)\s+([A-Za-z][\w\d]*?)\s*=\s*.+
	Match for pattern 2 : (int)\s+([A-Za-z][\w\d]*?)\s*=\s*.+
    ...

+ Timings:(prints disabled)
	File reading time: 68053ns
	JSON parsing time: 51378ns
	Regex extraction time: 35498486ns
	HS database compilation time (628 regex):1354825115ns
	Regex matching time (18 tests): 149797ns
	Regex matching time (avg): 8322ns
</code>