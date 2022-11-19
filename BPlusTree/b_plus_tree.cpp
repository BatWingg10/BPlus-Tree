#include "../include/b_plus_tree.h"
#include <iostream>
#include <stack>
#include <math.h>
using namespace std;

/*
 * Helper function to decide whether current b+tree is empty
 */
bool BPlusTree::IsEmpty() const
{

    if (root == NULL)
        return true;
    else
        return false;
}

/*****************************************************************************
 * SEARCH
 *****************************************************************************/
/*
 * Return the only value that associated with input key
 * This method is used for point query
 * @return : true means key exists
 */
int FindLeafNode(Node *node, const KeyType &key)
{

    int nodeSize = node->key_num;

    for (int i = 0; i < nodeSize; i++)
    {
        if (node->keys[i] > key)
        {
            return i;
        }
        else if (node->keys[i] == key)
        {
            return i + 1;
        }
    }

    return nodeSize;
}

bool BPlusTree::GetValue(const KeyType &key, RecordPointer &recordPointer)
{

    Node *leafNode = root; // start traversing from root
    int index = 0;

    while (!leafNode->is_leaf)
    {

        InternalNode *internalNode = (InternalNode *)leafNode;
        index = FindLeafNode(leafNode, key);
        leafNode = internalNode->children[index];
    }

    LeafNode *current = (LeafNode *)leafNode;
    for (int i = 0; i < current->key_num; i++)
    {
        if (current->keys[i] == key)
        {
            recordPointer = current->pointers[i];
            return true;
        }
    }

    return false;
}

/*****************************************************************************
 * INSERTION
 *****************************************************************************/
/*
 * Insert constant key & value pair into b+ tree
 * If current tree is empty, start new tree, otherwise insert into leaf Node.
 * @return: since we only support unique key, if user try to insert duplicate
 * keys return false, otherwise return true.
 */

void InsertLeafNodeValue(LeafNode *leafNode, KeyType key, int size, const RecordPointer &value)
{
    int i = size - 1;
    for (i = size - 1; i >= 0 && leafNode->keys[i] > key; i--)
    {
        leafNode->keys[i + 1] = leafNode->keys[i];
        leafNode->pointers[i + 1] = leafNode->pointers[i];
    }
    leafNode->key_num++;
    leafNode->keys[i + 1] = key;
    leafNode->pointers[i + 1] = value;
}

void InsertInternalNodeValue(InternalNode *internalNode, KeyType key, Node *newNode, Node *oldNode, KeyType passValue)
{
    int i = internalNode->key_num - 1;
    for (i = internalNode->key_num - 1; i >= 0 && internalNode->keys[i] > key; i--)
    {
        internalNode->keys[i + 1] = internalNode->keys[i];
        internalNode->children[i + 2] = internalNode->children[i + 1];
    }
    internalNode->key_num++;
    internalNode->keys[i + 1] = passValue;
    internalNode->children[i + 2] = newNode;
    internalNode->children[i + 1] = oldNode;
}

bool BPlusTree::Insert(const KeyType &key, const RecordPointer &value)
{

    RecordPointer current;

    if (root == NULL)
    { // If tree is empty
        LeafNode *leafNode = new LeafNode();
        leafNode->pointers[0] = value;
        leafNode->key_num += 1;
        leafNode->keys[0] = key;
        root = leafNode;
    }

    else
    {

        if (GetValue(key, current))
        { // check if it is a duplicate key
            return false;
        }

        Node *node = root;
        stack<InternalNode *> stack; // we maintain a stack to keep track of parent nodes

        while (!node->is_leaf)
        {
            InternalNode *internalNode = (InternalNode *)node;
            int index = FindLeafNode(node, key);
            stack.push((InternalNode *)node);
            node = internalNode->children[index];
        }

        LeafNode *leafNode = (LeafNode *)node;
        if (leafNode->key_num < MAX_FANOUT - 1)
        { // no overflow condition

            int nodeSize = leafNode->key_num;
            InsertLeafNodeValue(leafNode, key, nodeSize, value); // just insert and rearrange the node
        }
        else
        { // overflow condition

            LeafNode *newLeaf = new LeafNode();
            LeafNode *oldLeaf = (LeafNode *)leafNode;
            int oldLeafSize = oldLeaf->key_num;
            int newPtr = 0;
            int n = ceil((double)MAX_FANOUT / 2); // we split by right balancing the tree, hence ceiling value
            int index = FindLeafNode(oldLeaf, key);
            LeafNode *nextOfOld = oldLeaf->next_leaf;

            // Split Logic

            vector<KeyType> keys;
            vector<RecordPointer> recordPointers;
            int keysIndex = 0;
            for (int i = 0; i < oldLeafSize; i++)
            {
                keys.push_back(oldLeaf->keys[i]);
                recordPointers.push_back(oldLeaf->pointers[i]);
            }
            keys.insert(keys.begin() + index, key);
            recordPointers.insert(recordPointers.begin() + index, value);

            for (int i = 0; i < n; i++)
            {
                oldLeaf->keys[i] = keys[keysIndex];
                oldLeaf->pointers[i] = recordPointers[keysIndex++];
            }
            oldLeaf->key_num = n;

            for (int i = 0; i < keys.size() - n; i++)
            {
                newLeaf->keys[i] = keys[keysIndex];
                newLeaf->pointers[i] = recordPointers[keysIndex++];
            }
            newLeaf->key_num = keys.size() - n;

            oldLeaf->next_leaf = newLeaf;
            newLeaf->prev_leaf = oldLeaf;
            if (nextOfOld != NULL)
            {
                newLeaf->next_leaf = nextOfOld;
                nextOfOld->prev_leaf = newLeaf;
            }

            KeyType passValue = newLeaf->keys[0];

            if (stack.empty())
            {
                Node *par = NULL;

                if (par == NULL)
                {
                    InternalNode *internalNode = new InternalNode();
                    internalNode->children[0] = oldLeaf;
                    internalNode->children[1] = newLeaf;
                    internalNode->keys[0] = newLeaf->keys[0];
                    internalNode->key_num++;
                    root = internalNode;
                }
            }

            Node *newNode = (LeafNode *)newLeaf;
            Node *oldNode = (LeafNode *)oldLeaf;
            bool isElementAdded = false;

            while (!stack.empty())
            {
                InternalNode *parent = stack.top();
                stack.pop();
                InternalNode *internalNode = (InternalNode *)parent;
                InternalNode *newInternalNode;
                InternalNode *oldInternalNode;

                if (internalNode->key_num < MAX_FANOUT - 1)
                { // just insert and rearrange the node
                    InsertInternalNodeValue(internalNode, key, newNode, oldNode, passValue);
                    isElementAdded = true;
                    break;
                }
                else
                {
                    newInternalNode = new InternalNode();
                    oldInternalNode = (InternalNode *)parent;
                    int oldNodeSize = oldInternalNode->key_num;
                    int newptr = 0, newchildPtr = 0;
                    int N = ceil((double)MAX_FANOUT / 2);

                    int newKeyIndex = FindLeafNode(oldInternalNode, passValue);

                    bool isPassValueSet = false;
                    KeyType beforePassvalue = passValue;
                    Node *beforeOldNode;
                    Node *beforeNewNode;
                    vector<KeyType> keys;
                    vector<Node *> children(MAX_FANOUT + 1);
                    int index = 0, childIndex = 0, i;

                    for (i = 0; i < oldNodeSize; i++)
                    {
                        keys.push_back(oldInternalNode->keys[i]);
                        children[i] = (oldInternalNode->children[i]);
                    }
                    children[i] = (oldInternalNode->children[i]);

                    keys.insert(keys.begin() + newKeyIndex, passValue);
                    int j = oldNodeSize;
                    while (j > newKeyIndex)
                    {
                        children[j + 1] = children[j];
                        j--;
                    }
                    children[newKeyIndex] = oldNode;
                    children[newKeyIndex + 1] = newNode;

                    for (i = 0; i < N; i++)
                    {
                        oldInternalNode->keys[i] = keys[index++];
                        oldInternalNode->children[i] = children[childIndex++];
                    }
                    oldInternalNode->children[i] = children[childIndex++];
                    passValue = keys[index++];
                    oldInternalNode->key_num = N;

                    for (i = oldInternalNode->key_num + 1; i < MAX_FANOUT; i++)
                    {
                        oldInternalNode->children[i] = NULL;
                    }
                    oldNode = oldInternalNode;

                    for (i = 0; i < keys.size() - N - 1; i++)
                    {
                        newInternalNode->keys[i] = keys[index++];
                        newInternalNode->children[i] = children[childIndex++];
                    }
                    newInternalNode->children[i] = children[childIndex++];
                    newInternalNode->key_num = keys.size() - N - 1;

                    newNode = newInternalNode;
                }
            }

            if (!isElementAdded)
            {
                InternalNode *node = new InternalNode();
                node->keys[0] = passValue;
                node->children[0] = oldNode;
                node->children[1] = newNode;
                node->key_num++;
                root = node;
            }
        }
    }

    return true;
}


/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/*
 * Delete key & value pair associated with input key
 * If current tree is empty, return immdiately.
 * If not, User needs to first find the right leaf node as deletion target, then
 * delete entry from leaf node. Remember to deal with redistribute or merge if
 * necessary.
 */
void BPlusTree::Remove(const KeyType &key)
{

    // Driver function

    Node *leaf = root;
    if (root == NULL)
        return;
    int childIndx;
    while (!leaf->is_leaf)
    {
        InternalNode *intrlNode = (InternalNode *)leaf;
        leaf = intrlNode->children[0];
    }
    LeafNode *node = (LeafNode *)leaf;

    root = NULL;
    Node *mergeNode;

    int cIndx = node->key_num;

    while (node != NULL)
    {

        for (int i = 0; i < node->key_num; i++)
        {
            if (node->keys[i] != key)
            {
                RemoveKey(node->keys[i], node->pointers[i]); // actual function to remove the key
            }
        }
        node = node->next_leaf;
    }
}

bool BPlusTree::RemoveKey(const KeyType &key, const RecordPointer &value)
{
    RecordPointer current;

    if (root == NULL)
    { // If tree is empty
        LeafNode *leafNode = new LeafNode();
        leafNode->pointers[0] = value;
        leafNode->key_num += 1;
        leafNode->keys[0] = key;
        root = leafNode;
    }

    else
    {

        if (GetValue(key, current))
        { // check if it is a duplicate key
            return false;
        }

        Node *node = root;
        stack<InternalNode *> stack; // we maintain a stack to keep track of parent nodes

        while (!node->is_leaf)
        {
            InternalNode *internalNode = (InternalNode *)node;
            int index = FindLeafNode(node, key);
            stack.push((InternalNode *)node);
            node = internalNode->children[index];
        }

        LeafNode *leafNode = (LeafNode *)node;
        if (leafNode->key_num < MAX_FANOUT - 1)
        { // no overflow condition

            int nodeSize = leafNode->key_num;
            InsertLeafNodeValue(leafNode, key, nodeSize, value); // just insert and rearrange the node
        }
        else
        { // overflow condition

            LeafNode *newLeaf = new LeafNode();
            LeafNode *oldLeaf = (LeafNode *)leafNode;
            int oldLeafSize = oldLeaf->key_num;
            int newPtr = 0;
            int n = ceil((double)MAX_FANOUT / 2); // we split by right balancing the tree, hence ceiling value
            int index = FindLeafNode(oldLeaf, key);
            LeafNode *nextOfOld = oldLeaf->next_leaf;

            // Split Logic

            vector<KeyType> keys;
            vector<RecordPointer> recordPointers;
            int keysIndex = 0;
            for (int i = 0; i < oldLeafSize; i++)
            {
                keys.push_back(oldLeaf->keys[i]);
                recordPointers.push_back(oldLeaf->pointers[i]);
            }
            keys.insert(keys.begin() + index, key);
            recordPointers.insert(recordPointers.begin() + index, value);

            for (int i = 0; i < n; i++)
            {
                oldLeaf->keys[i] = keys[keysIndex];
                oldLeaf->pointers[i] = recordPointers[keysIndex++];
            }
            oldLeaf->key_num = n;

            for (int i = 0; i < keys.size() - n; i++)
            {
                newLeaf->keys[i] = keys[keysIndex];
                newLeaf->pointers[i] = recordPointers[keysIndex++];
            }
            newLeaf->key_num = keys.size() - n;

            oldLeaf->next_leaf = newLeaf;
            newLeaf->prev_leaf = oldLeaf;
            if (nextOfOld != NULL)
            {
                newLeaf->next_leaf = nextOfOld;
                nextOfOld->prev_leaf = newLeaf;
            }

            KeyType passValue = newLeaf->keys[0];

            if (stack.empty())
            {
                Node *par = NULL;

                if (par == NULL)
                {
                    InternalNode *internalNode = new InternalNode();
                    internalNode->children[0] = oldLeaf;
                    internalNode->children[1] = newLeaf;
                    internalNode->keys[0] = newLeaf->keys[0];
                    internalNode->key_num++;
                    root = internalNode;
                }
            }

            Node *newNode = (LeafNode *)newLeaf;
            Node *oldNode = (LeafNode *)oldLeaf;
            bool isElementAdded = false;

            while (!stack.empty())
            {
                InternalNode *parent = stack.top();
                stack.pop();
                InternalNode *internalNode = (InternalNode *)parent;
                InternalNode *newInternalNode;
                InternalNode *oldInternalNode;

                if (internalNode->key_num < MAX_FANOUT - 1)
                { // just insert and rearrange the node
                    InsertInternalNodeValue(internalNode, key, newNode, oldNode, passValue);
                    isElementAdded = true;
                    break;
                }
                else
                {
                    newInternalNode = new InternalNode();
                    oldInternalNode = (InternalNode *)parent;
                    int oldNodeSize = oldInternalNode->key_num;
                    int newptr = 0, newchildPtr = 0;
                    int N = ceil((double)MAX_FANOUT / 2);

                    int newKeyIndex = FindLeafNode(oldInternalNode, passValue);

                    bool isPassValueSet = false;
                    KeyType beforePassvalue = passValue;
                    Node *beforeOldNode;
                    Node *beforeNewNode;
                    vector<KeyType> keys;
                    vector<Node *> children(MAX_FANOUT + 1);
                    int index = 0, childIndex = 0, i;

                    for (i = 0; i < oldNodeSize; i++)
                    {
                        keys.push_back(oldInternalNode->keys[i]);
                        children[i] = (oldInternalNode->children[i]);
                    }
                    children[i] = (oldInternalNode->children[i]);

                    keys.insert(keys.begin() + newKeyIndex, passValue);
                    int j = oldNodeSize;
                    while (j > newKeyIndex)
                    {
                        children[j + 1] = children[j];
                        j--;
                    }
                    children[newKeyIndex] = oldNode;
                    children[newKeyIndex + 1] = newNode;

                    for (i = 0; i < N; i++)
                    {
                        oldInternalNode->keys[i] = keys[index++];
                        oldInternalNode->children[i] = children[childIndex++];
                    }
                    oldInternalNode->children[i] = children[childIndex++];
                    passValue = keys[index++];
                    oldInternalNode->key_num = N;

                    for (i = oldInternalNode->key_num + 1; i < MAX_FANOUT; i++)
                    {
                        oldInternalNode->children[i] = NULL;
                    }
                    oldNode = oldInternalNode;

                    for (i = 0; i < keys.size() - N - 1; i++)
                    {
                        newInternalNode->keys[i] = keys[index++];
                        newInternalNode->children[i] = children[childIndex++];
                    }
                    newInternalNode->children[i] = children[childIndex++];
                    newInternalNode->key_num = keys.size() - N - 1;

                    newNode = newInternalNode;
                }
            }

            if (!isElementAdded)
            {
                InternalNode *node = new InternalNode();
                node->keys[0] = passValue;
                node->children[0] = oldNode;
                node->children[1] = newNode;
                node->key_num++;
                root = node;
            }
        }
    }

    return true;
}

/*****************************************************************************
 * RANGE_SCAN
 *****************************************************************************/
/*
 * Return the values that within the given key range
 * First find the node large or equal to the key_start, then traverse the leaf
 * nodes until meet the key_end position, fetch all the records.
 */
void BPlusTree::RangeScan(const KeyType &key_start, const KeyType &key_end,
                          std::vector<RecordPointer> &result)
{

    Node *leaf = root;
    if (root == NULL)
        return;
    int childIndx;
    while (!leaf->is_leaf)
    {
        InternalNode *internalNode = (InternalNode *)leaf;
        childIndx = FindLeafNode(leaf, key_start);
        leaf = internalNode->children[childIndx];
    }
    LeafNode *node = (LeafNode *)leaf;

    while (node != NULL)
    {
        for (int indx = 0; indx < node->key_num && node->keys[indx] < key_end; indx++)
        {
            if (node->keys[indx] >= key_start && node->keys[indx] < key_end)
            {
                result.push_back(node->pointers[indx]);
            }
        }
        node = node->next_leaf;
    }
}
 
