#include <linked_list.h>

static void remove(
  struct dpawin_list_entry* entry
){
  if(!entry->list)
    return;
  if(entry->next)
    entry->next->previous = entry->previous;
  if(entry->previous)
    entry->previous->next = entry->next;
  if(entry->list->first == entry)
    entry->list->first = entry->next;
  if(entry->list->last == entry)
    entry->list->last = entry->previous;
  entry->list = 0;
  entry->next = 0;
  entry->previous = 0;
}

bool dpawin_linked_list_set(
  struct dpawin_list* list,
  struct dpawin_list_entry* entry,
  struct dpawin_list_entry* before
){
  if(before && list && before->list != list)
    return false;
  if((before == entry || (before && before == entry->next)) && list)
    return true;
  remove(entry);
  if(!list)
    return true;
  if(before)
    list = before->list;
  entry->list = list;
  if(!list->first){
    list->first = entry;
    list->last  = entry;
    return true;
  }
  if(!before)
    before = list->first;
  entry->next = before;
  entry->previous = before->previous;
  if(before->previous)
    before->previous->next = entry;
  before->previous = entry;
  if(!entry->previous)
    list->first = entry;
  if(!entry->next)
    list->last = entry;
  return true;
}
