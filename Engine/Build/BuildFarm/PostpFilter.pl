#!/usr/bin/env perl

while(<>)
{
	# remove timestamps added by gubp
	s/^\[[0-9:.]+\] //;

	# remove the mono prefix for recursive UAT calls on Mac
	s/^mono: ([A-Za-z0-9-]+: )/$1/;

	# remove the AutomationTool prefix for recursive UAT calls anywhere
	s/^AutomationTool: ([A-Za-z0-9-]+: )/$1/;
	
	# remove the gubp function or prefix
	s/^([A-Za-z0-9.-]+): //;

	# if it was the editor, also strip any additional timestamp
	if($1 && $1 =~ /^UE4Editor/)
	{
		s/^\[[0-9.:-]+\]\[\s*\d+\]//;
	}
	
	# output the line
	print $_;
}
