# Language
A concept of a language compiler generator based of json instruction files
<br>
Requires a local compilation of intel <a href="https://github.com/intel/hyperscan">hyperscan</a>*
<br>
<br>
* Note: Until fixed, remember to <a href="https://github.com/intel/hyperscan/issues/292#issuecomment-762635447">Fix hyperscan</a>
<br>
Results: (timings in the end)
<br>
<br>
if(var1&gt;10):
Start scan:
	Match for pattern 377 : if\s*\(\s*(not\s)?\s*([A-Za-z][\w\d]*?)(\s*&gt;\s*(not\s)?\s*\d+\s*)+\s*\)\s*:
<br>
(Not working yet, known bug)
if  (  var1  &gt; 10  and 3&lt;40):
Start scan:
<br>
(Not working yet, known bug)
if  (  var1  &gt; 10  and var2 == var3 and 3&lt;40  )  :
Start scan:
	Match for pattern 1 : ([A-Za-z][\w\d]*?)\s*=\s*.+
	Match for pattern 8 : (\w+)\s+([A-Za-z][\w\d]*?)\s*=\s*.+
<br>
if var1&gt;10:
Start scan:
	Match for pattern 289 : if\s+(not\s)?\s*([A-Za-z][\w\d]*?)(\s*&gt;\s*(not\s)?\s*\d+\s*)+\s*:
<br>
(Not working yet, known bug)
if    var1  &gt; 10  and 3&lt;40:
Start scan:
<br>
(Not working yet, known bug)
if    var1  &gt; 10  and var2 == var3 and 3&lt;40   :
Start scan:
	Match for pattern 1 : ([A-Za-z][\w\d]*?)\s*=\s*.+
	Match for pattern 8 : (\w+)\s+([A-Za-z][\w\d]*?)\s*=\s*.+
<br>
while(var1&gt;10):
Start scan:
	Match for pattern 553 : while\s*\(\s*(not\s)?\s*([A-Za-z][\w\d]*?)(\s*&gt;\s*(not\s)?\s*\d+\s*)+\s*\)\s*:
<br>
(Not working yet, known bug)
while  (  var1  &gt; 10  and 3&lt;40):
Start scan:
<br>
(Not working yet, known bug)
while  (  var1  &gt; 10  and var2 == var3 and 3&lt;40  )   :
Start scan:
	Match for pattern 1 : ([A-Za-z][\w\d]*?)\s*=\s*.+
	Match for pattern 8 : (\w+)\s+([A-Za-z][\w\d]*?)\s*=\s*.+
<br>
while var1&gt;10:
Start scan:
	Match for pattern 465 : while\s+(not\s)?\s*([A-Za-z][\w\d]*?)(\s*&gt;\s*(not\s)?\s*\d+\s*)+\s*:
<br>
(Not working yet, known bug)
while    var1  &gt; 10  and 3&lt;40:
Start scan:
<br>
(Not working yet, known bug)
while   var1  &gt; 10  and var2 == var3   and    3&lt;40    :
Start scan:
	Match for pattern 1 : ([A-Za-z][\w\d]*?)\s*=\s*.+
	Match for pattern 8 : (\w+)\s+([A-Za-z][\w\d]*?)\s*=\s*.+
<br>
a    =     10
Start scan:
	Match for pattern 1 : ([A-Za-z][\w\d]*?)\s*=\s*.+
<br>
a=19
Start scan:
	Match for pattern 1 : ([A-Za-z][\w\d]*?)\s*=\s*.+
<br>
int   a   =  10
Start scan:
	*Match for pattern 2 : (int)\s+([A-Za-z][\w\d]*?)\s*=\s*.+
	Match for pattern 1 : ([A-Za-z][\w\d]*?)\s*=\s*.+
	Match for pattern 8 : (\w+)\s+([A-Za-z][\w\d]*?)\s*=\s*.+
<br>
int a=19
Start scan:
	*Match for pattern 2 : (int)\s+([A-Za-z][\w\d]*?)\s*=\s*.+
	Match for pattern 1 : ([A-Za-z][\w\d]*?)\s*=\s*.+
	Match for pattern 8 : (\w+)\s+([A-Za-z][\w\d]*?)\s*=\s*.+
<br>
const    int    a   = 123
Start scan:
	Match for pattern 34 : (const\s+)+(\w+)\s+([A-Za-z][\w\d]*?)\s*=\s*.+
	*Match for pattern 10 : (const\s+)+(int)\s+([A-Za-z][\w\d]*?)\s*=\s*.+
	Match for pattern 2 : (int)\s+([A-Za-z][\w\d]*?)\s*=\s*.+
<br>
const int a=124
Start scan:
	Match for pattern 34 : (const\s+)+(\w+)\s+([A-Za-z][\w\d]?)\s*=\s*.+
	*Match for pattern 10 : (const\s+)+(int)\s+([A-Za-z][\w\d]*?)\s*=\s*.+
	Match for pattern 2 : (int)\s+([A-Za-z][\w\d]*?)\s*=\s*.+
    ...
<br>
+ Timings:(prints disabled)
	File reading time: Depends of language file sizes
	JSON parsing time: Depends of language file sizes
	Regex extraction time: 1905308ns
	HS database compilation time (n regex): 1203183ns
	Regex matching time (18 tests): 35210ns
	Regex matching time (avg): 1956ns