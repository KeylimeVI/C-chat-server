#ifndef WORD_LIST_H
#define WORD_LIST_H

char **prune_word_list(char **word_list, int len);
void deallocate_pruned_word_list(char **word_list);

#endif