#include <stdio.h>
#include <sys/time.h>
#include <math.h>
#include <stdlib.h>
#include "sequence_alignment.h"
#include "sequence_compression.h"
#include "sand_align_macros.h"

#define MIN_ALIGN 40

seq get_next_sequence_wrapper(FILE * input)
{
#ifdef COMPRESSION
	cseq c; c = get_next_cseq(input); return uncompress_seq(c);
#else
	return get_next_sequence(input);
#endif
}

int main(int argc, char ** argv)
{
	FILE * input;
	seq s1, s2;
	char ori;

	if (argc == 2)
	{
		input = fopen(argv[1], "r");
		if (!input)
		{
			fprintf(stderr, "ERROR: Could not open file %s for reading.\n", argv[1]);
			exit(1);
		}
	}
	else
	{
		input = stdin;
	}

	s1 = get_next_sequence_wrapper(input);

	print_OVL_envelope_start(stdout);
	
	while (!feof(input))
	{
#ifdef SPEEDTEST
		gettimeofday(&start_tv, NULL);
#endif
		s2 = get_next_sequence_wrapper(input);

		// If the sequence is null, we reached an end of file, and need to work
		// with a new one.
		if (s2.seq == 0)
		{
			free_seq(s1);
			free_seq(s2);
			s1 = get_next_sequence_wrapper(input);
			continue;
		}

		//print_sequence(stdout, s1);
		//print_sequence(stdout, s2);
		if (atoi(s2.metadata) == -1) { revcomp(&s2); ori = 'I'; }
		else { ori = 'N'; }

		delta tb = prefix_suffix_align(s1.seq, s2.seq, MIN_ALIGN);
		tb.ori = ori;
		//fprintf(stderr, "score: %d\n", tb.score);
	//print_alignment(stderr, s1.seq, s2.seq, tb, 80);

		//tb.ori = 'N';
		//revcomp(&s2);
		//delta tb_r = prefix_suffix_align(s1.seq, s2.seq, MIN_ALIGN);
		//tb_r.ori = 'I';

		// A lower score is better
		//if (tb.quality <= tb_r.quality)
		//{
		if (tb.quality <= 0.04)
		{
			print_OVL_message(stdout, tb, s1.id, s2.id);
		}
		//}
		//else
		//{
		//	print_OVL_message(stdout, tb_r, s1.id, s2.id);
		//}

		//print_delta(stdout, tb, s1.id, s2.id, 1);
		//print_OVL_message(stdout, tb, s1.id, s2.id);

		free_seq(s2);
		free_delta(tb);

#ifdef SPEEDTEST
		gettimeofday(&end_tv, NULL);
		time_diff = (end_tv.tv_sec+ (end_tv.tv_usec/1000000.0)) - (start_tv.tv_sec+ (start_tv.tv_usec/1000000.0));
		total_time += time_diff;
		count++;

		if (count % 1000 == 0)
		{
			fprintf(stderr, "%d\t%0.6f\n",count, total_time / (float) count);
		}
#endif
	}
#ifdef SPEEDTEST
	fprintf(stderr, "%d\t%0.6f\n",count, total_time/(float)count);
#endif

	free_seq(s1);
	fclose(input);
	print_OVL_envelope_end(stdout);
	return 0;
}



