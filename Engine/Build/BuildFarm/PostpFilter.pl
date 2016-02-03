#!/usr/bin/env perl

while(<>)
{
	# remove timestamps added by gubp
	s/^\[[0-9:.]+\] //;
	
	# remove the gubp function or prefix
	s/^([A-Za-z0-9.-]+): //;

	# if it was the editor, also strip any additional timestamp
	if($1 && $1 eq 'UE4Editor-Cmd')
	{
		s/^\[[0-9.:-]+\]\[\s*\d+\]//;
	}
	
	# output the line
	print $_;
}
