
#ifndef LOG_OP_CC
#define LOG_OP_CC

#include "MyDB_LogicalOps.h"
#include "Aggregate.h"
#include "BPlusSelection.h"
#include "RegularSelection.h"
#include "ScanJoin.h"
#include "SortMergeJoin.h"


MyDB_TableReaderWriterPtr LogicalTableScan :: execute (map <string, MyDB_TableReaderWriterPtr> &allTableReaderWriters,
	map <string, MyDB_BPlusTreeReaderWriterPtr> &allBPlusReaderWriters) {

	MyDB_BufferManagerPtr bufferMgr = nullptr;
	string storageLoc = inputSpec->getStorageLoc ();
	for (auto &entry : allTableReaderWriters) {
		if (entry.second->getTable ()->getStorageLoc () == storageLoc) {
			bufferMgr = entry.second->getBufferMgr ();
			break;
		}
	}


	MyDB_TableReaderWriterPtr inputTable = make_shared <MyDB_TableReaderWriter> (inputSpec, bufferMgr);
	

	MyDB_TableReaderWriterPtr outputTable = make_shared <MyDB_TableReaderWriter> (outputSpec, bufferMgr);

	string selectionPredicate;
	if (selectionPred.size () == 0) {
		selectionPredicate = "bool[true]";
	} else {
		selectionPredicate = selectionPred[0]->toString ();
		for (size_t i = 1; i < selectionPred.size (); i++) {
			selectionPredicate = "&& (" + selectionPredicate + ", " + selectionPred[i]->toString () + ")";
		}
	}

	vector <string> projections;
	for (auto &att : outputSpec->getSchema ()->getAtts ()) {
		projections.push_back ("[" + att.first + "]");
	}

	RegularSelection mySelection (inputTable, outputTable, selectionPredicate, projections);
	mySelection.run ();

	return outputTable;
}

MyDB_TableReaderWriterPtr LogicalJoin :: execute (map <string, MyDB_TableReaderWriterPtr> &allTableReaderWriters,
	map <string, MyDB_BPlusTreeReaderWriterPtr> &allBPlusReaderWriters) {
	
	MyDB_TableReaderWriterPtr leftTable = leftInputOp->execute (allTableReaderWriters, allBPlusReaderWriters);
	MyDB_TableReaderWriterPtr rightTable = rightInputOp->execute (allTableReaderWriters, allBPlusReaderWriters);

	MyDB_TableReaderWriterPtr outputTable = make_shared <MyDB_TableReaderWriter> (outputSpec, leftTable->getBufferMgr ());

	string finalSelectionPredicate;
	if (outputSelectionPredicate.size () == 0) {
		finalSelectionPredicate = "bool[true]";
	} else {
		finalSelectionPredicate = outputSelectionPredicate[0]->toString ();
		for (size_t i = 1; i < outputSelectionPredicate.size (); i++) {
			finalSelectionPredicate = "&& (" + finalSelectionPredicate + ", " + outputSelectionPredicate[i]->toString () + ")";
		}
	}

	vector <pair <string, string>> equalityChecks;
	for (auto &pred : outputSelectionPredicate) {
		if (pred->isEq ()) {
			ExprTreePtr lhs = pred->getLHS ();
			ExprTreePtr rhs = pred->getRHS ();
			

			bool lhsInLeft = false, lhsInRight = false;
			bool rhsInLeft = false, rhsInRight = false;
			
			for (auto &att : leftInputOp->getOutputTable ()->getSchema ()->getAtts ()) {
				
				size_t pos = att.first.find ('_');
				if (pos != string::npos) {
					string tableAlias = att.first.substr (0, pos);
					if (lhs->referencesTable (tableAlias)) lhsInLeft = true;
					if (rhs->referencesTable (tableAlias)) rhsInLeft = true;
				}
			}
			
			for (auto &att : rightInputOp->getOutputTable ()->getSchema ()->getAtts ()) {
				size_t pos = att.first.find ('_');
				if (pos != string::npos) {
					string tableAlias = att.first.substr (0, pos);
					if (lhs->referencesTable (tableAlias)) lhsInRight = true;
					if (rhs->referencesTable (tableAlias)) rhsInRight = true;
				}
			}
			
			
			if (lhsInLeft && rhsInRight) {
				equalityChecks.push_back (make_pair (lhs->toString (), rhs->toString ()));
			} else if (lhsInRight && rhsInLeft) {
				
				equalityChecks.push_back (make_pair (rhs->toString (), lhs->toString ()));
			}
		}
	}

	
	vector <string> projections;
	for (auto &att : outputSpec->getSchema ()->getAtts ()) {
		projections.push_back ("[" + att.first + "]");
	}

	ScanJoin myJoin (leftTable, rightTable, outputTable, finalSelectionPredicate, projections,
		equalityChecks, "bool[true]", "bool[true]");
	myJoin.run ();


	leftTable->getBufferMgr ()->killTable (leftTable->getTable ());
	rightTable->getBufferMgr ()->killTable (rightTable->getTable ());

	return outputTable;
}

#endif
