# Language
A concept of a language compiler generator based of json instruction files
<br>
<br>
NOTE: Everithing in this README is REALLY updated. I don't use Hyperscan anymore
<br>
https://godbolt.org/z/sG1vE9jdY
<br>
<br>
Requires a local compilation of intel <a href="https://github.com/intel/hyperscan">hyperscan</a>*
<br>
<br>
* Note: Until fixed, remember to <a href="https://github.com/intel/hyperscan/issues/292#issuecomment-762635447">Fix hyperscan</a>
<br>
Results: (timings in the end)
<br>
<br>
<hr>
if(var1&gt;10):<br>
Start scan:<br>
&emsp;Match for pattern 377 : if\s*\(\s*(not\s)?\s*([A-Za-z][\w\d]*?)(\s*&gt;\s*(not\s)?\s*\d+\s*)+\s*\)\s*:<br>
<br>
(Not working yet, known bug)<br>
if  (  var1  &gt; 10  and 3&lt;40):<br>
Start scan:<br>
<br>
(Not working yet, known bug)<br>
if  (  var1  &gt; 10  and var2 == var3 and 3&lt;40  )  :<br>
Start scan:<br>
&emsp;Match for pattern 1 : ([A-Za-z][\w\d]*?)\s*=\s*.+<br>
&emsp;Match for pattern 8 : (\w+)\s+([A-Za-z][\w\d]*?)\s*=\s*.+<br>
<br>
if var1&gt;10:<br>
Start scan:<br>
&emsp;Match for pattern 289 : if\s+(not\s)?\s*([A-Za-z][\w\d]*?)(\s*&gt;\s*(not\s)?\s*\d+\s*)+\s*:<br>
<br>
(Not working yet, known bug)<br>
if    var1  &gt; 10  and 3&lt;40:<br>
Start scan:<br>
<br>
(Not working yet, known bug)<br>
if    var1  &gt; 10  and var2 == var3 and 3&lt;40   :<br>
Start scan:<br>
&emsp;Match for pattern 1 : ([A-Za-z][\w\d]*?)\s*=\s*.+<br>
&emsp;Match for pattern 8 : (\w+)\s+([A-Za-z][\w\d]*?)\s*=\s*.+<br>
<br>
while(var1&gt;10):<br>
Start scan:<br>
&emsp;Match for pattern 553 : while\s*\(\s*(not\s)?\s*([A-Za-z][\w\d]*?)(\s*&gt;\s*(not\s)?\s*\d+\s*)+\s*\)\s*:<br>
<br>
(Not working yet, known bug)<br>
while  (  var1  &gt; 10  and 3&lt;40):<br>
Start scan:<br>
<br>
(Not working yet, known bug)<br>
while  (  var1  &gt; 10  and var2 == var3 and 3&lt;40  )   :<br>
Start scan:<br>
&emsp;Match for pattern 1 : ([A-Za-z][\w\d]*?)\s*=\s*.+<br>
&emsp;Match for pattern 8 : (\w+)\s+([A-Za-z][\w\d]*?)\s*=\s*.+<br>
<br>
while var1&gt;10:<br>
Start scan:<br>
&emsp;Match for pattern 465 : while\s+(not\s)?\s*([A-Za-z][\w\d]*?)(\s*&gt;\s*(not\s)?\s*\d+\s*)+\s*:<br>
<br>
(Not working yet, known bug)<br>
while    var1  &gt; 10  and 3&lt;40:<br>
Start scan:<br>
<br>
(Not working yet, known bug)<br>
while   var1  &gt; 10  and var2 == var3   and    3&lt;40    :<br>
Start scan:<br>
&emsp;Match for pattern 1 : ([A-Za-z][\w\d]*?)\s*=\s*.+<br>
&emsp;Match for pattern 8 : (\w+)\s+([A-Za-z][\w\d]*?)\s*=\s*.+<br>
<br>
a    =     10<br>
Start scan:<br>
&emsp;Match for pattern 1 : ([A-Za-z][\w\d]*?)\s*=\s*.+<br>
<br>
a=19<br>
Start scan:<br>
&emsp;Match for pattern 1 : ([A-Za-z][\w\d]*?)\s*=\s*.+<br>
<br>
int   a   =  10<br>
Start scan:<br>
&emsp;*Match for pattern 2 : (int)\s+([A-Za-z][\w\d]*?)\s*=\s*.+<br>
&emsp;Match for pattern 1 : ([A-Za-z][\w\d]*?)\s*=\s*.+<br>
&emsp;Match for pattern 8 : (\w+)\s+([A-Za-z][\w\d]*?)\s*=\s*.+<br>
<br>
int a=19<br>
Start scan:<br>
&emsp;*Match for pattern 2 : (int)\s+([A-Za-z][\w\d]*?)\s*=\s*.+<br>
&emsp;Match for pattern 1 : ([A-Za-z][\w\d]*?)\s*=\s*.+<br>
&emsp;Match for pattern 8 : (\w+)\s+([A-Za-z][\w\d]*?)\s*=\s*.+<br>
<br>
const    int    a   = 123<br>
Start scan:<br>
&emsp;Match for pattern 34 : (const\s+)+(\w+)\s+([A-Za-z][\w\d]*?)\s*=\s*.+<br>
&emsp;*Match for pattern 10 : (const\s+)+(int)\s+([A-Za-z][\w\d]*?)\s*=\s*.+<br>
&emsp;Match for pattern 2 : (int)\s+([A-Za-z][\w\d]*?)\s*=\s*.+<br>
<br>
const int a=124<br>
Start scan:<br>
&emsp;Match for pattern 34 : (const\s+)+(\w+)\s+([A-Za-z][\w\d]?)\s*=\s*.+<br>
&emsp;*Match for pattern 10 : (const\s+)+(int)\s+([A-Za-z][\w\d]*?)\s*=\s*.+<br>
&emsp;Match for pattern 2 : (int)\s+([A-Za-z][\w\d]*?)\s*=\s*.+<br>
&emsp;...<br>
<br>
+ Timings: (prints disabled)<br>
&emsp;File reading time: Depends of language file sizes<br>
&emsp;JSON parsing time: Depends of language file sizes<br>
&emsp;Regex extraction time: 1905308ns<br>
&emsp;HS database compilation time (n regex): 1203183ns<br>
&emsp;Regex matching time (18 tests): 35210ns<br>
&emsp;Regex matching time (avg): 1956ns<br>
<hr>
