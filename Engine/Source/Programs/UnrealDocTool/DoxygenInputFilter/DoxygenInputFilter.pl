#!/usr/bin/perl -w
#use strict;

# Find the first "//" or "/*" on the line.
# If it's immediately followed by "~" (our "don't comment" code), ignore it.
# If it's a "///" or "/**" (Doxygen's "comment" code), ignore it.
# Otherwise, transform it into the appropriate Doxygen "comment" code.
# For .cpp files, ignore all comment-modification code as it's causing undesired comments (e.g. copyright notices) to be captured.
# Convert "GENERATED_UCLASS_BODY()" macros to "GENERATED_BODY()", set access level to public, and declare the old-style UE4 constructor, all on the same line.

my $line = "";
my $lastclassname;
my $commentposition;
my $tempvalue;
open(FILE1, $ARGV[0]) || die "  error: $!\n";
while (<FILE1>)
{
    $line = $_;
	
	if ($line =~ /\/\/ Copyright .+ Epic Games, Inc. All Rights Reserved./)
	{
		# Discard copyright line.
		$line = " \n";
	}
	else
	{
		if (index($line, "class ") eq 0)
		{
			if ($line =~ /class \w+_API \w+/)
			{
				$tempvalue = index($line, "_API") + 5;
				if (index($line, ":") >= 0)
				{
					$lastclassname = substr($line, $tempvalue, index($line, ":") - $tempvalue);
				}
				else
				{
					$lastclassname = substr($line, $tempvalue);
				}
#print "lcn0 = " . $lastclassname . "\n";
			}
			elsif (index($line, ":") >= 0)
			{
				$tempvalue = index($line, "class ") + 6;
				$lastclassname = substr($line, $tempvalue, index($line, ":") - $tempvalue);
#print "lcn1 = " . $lastclassname . "\n";
			}
			else
			{
				# This will catch forward-declared classes, but should not be a problem because these should never immediately precede a GENERATED_UCLASS_BODY() line.
				$lastclassname = substr($line, index($line, "class ") + 6);
#print "lcn2 = " . $lastclassname . "\n";
			}
		}
		else
		{
			if (index($line, "GENERATED_UCLASS_BODY()") >= 0)
			{
				# $lastclassname might not be initialized. If so, the previous logic block failed to catch a class name, or "GENERATED_UCLASS_BODY()" is floating around loose in a file somewhere.
				$line = "GENERATED_BODY() public: " . $lastclassname . "(const FObjectInitializer& ObjectInitializer);\n";
			}
		}
		$commentposition = index($line, "\/\/");
		$tempvalue = index($line, "\/*");
		if ($commentposition < 0)
		{
			$commentposition = $tempvalue;
		}
		elsif ($tempvalue >= 0)
		{
			$commentposition = $commentposition < $tempvalue ? $commentposition : $tempvalue;
		}
		
		if ($commentposition >= 0)
		{
			if ((length $line) > ($commentposition + 2))
			{
				$tempvalue = substr($line, $commentposition + 2, 1);
				if (!($tempvalue eq "~") && !($tempvalue eq substr($line, $commentposition + 1, 1)))
				{
					substr($line, $commentposition + 1, 0) = substr($line, $commentposition + 1, 1);
				}
			}
			# This elsif indicates an error - the line didn't end with "\n".
			# If we find this happening, we should start appending "\n" on our own.
			#elsif ((length $line) > ($commentposition + 1))
			#{
			#    substr($line, $commentposition + 1, 0) = substr($line, $commentposition + 1, 1);
			#}
		}
	}
	print $line;
}
