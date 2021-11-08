Here goes code relating special cases in Definition files for two
languages. As so, For translating javascript->java you don't need to go
through binary code, just use the different definitions, and here
state any special case. For example, variables in javascript do not
have to be declared, but in java they do.
This case would not be necessary, but as an example:
This could also be used to modify the definition file, so that
there can be added restrictions

This could be stated like this:
// javascript-java.json
{
    "standalones_var":{
        type:{
            group_input:1,
            group_output:0,

            default:"(determine)"
        }
    }
}