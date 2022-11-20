package com.ankitsultana;

import javax.annotation.Nullable;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;

public class Main {
    public static void main(String[] args) {
        if (args.length != 2) {
            System.out.println(args.length);
            System.err.println("usage: (-testStringOverhead | -testArrayUpsertInfo | -testArrayListUpsertInfo | -testCHPUpsertInfo) numItems");
            return;
        }
        int numItems = Integer.parseInt(args[1].replaceAll(",", "").replaceAll("_", ""));
        if (args[0].equalsIgnoreCase("testStringOverhead")) {
            testStringOverhead(numItems);
        } else if (args[0].equalsIgnoreCase("testArrayUpsertInfo")) {
            testArrayUpsertInfo(numItems);
        } else if (args[0].equalsIgnoreCase("testArrayListUpsertInfo")) {
            testArrayListUpsertInfo(numItems);
        } else if (args[0].equalsIgnoreCase("testHPUpsertInfo")) {
            testHPUpsertInfo(numItems);
        } else if (args[0].equalsIgnoreCase("testCHPUpsertInfo")) {
            testCHPUpsertInfo(numItems);
        } else {
            System.err.println("usage: (-testStringOverhead | -testArrayUpsertInfo | -testArrayListUpsertInfo | -testCHPUpsertInfo) numItems");
        }
    }

    private static void testStringOverhead(int numItems) {
        System.out.println("Length of 1 UUID: " + UUID.randomUUID().toString().length());
        System.out.println("Number of Items:  " + numItems);
        String[] s = new String[numItems];
        for (int i = 0; i < s.length; i++) {
            s[i] = UUID.randomUUID().toString();
        }
    }

    private static void testArrayUpsertInfo(int numItems) {
        System.out.println("Simulating storing upsert info in vanilla array");
        RecordInfo[] records = new RecordInfo[numItems];
        PrimaryKey[] uuidList = new PrimaryKey[numItems];
        for (int i = 0; i < numItems; i++) {
            uuidList[i] = create();
            records[i] = new RecordInfo(i, (long) i);
        }
    }

    private static void testArrayListUpsertInfo(int numItems) {
        System.out.println("Simulating storing upsert info in array-list");
        List<PrimaryKey> uuidList = new ArrayList<>(numItems);
        List<RecordInfo> records = new ArrayList<>(numItems);
        for (int i = 0; i < numItems; i++) {
            uuidList.add(create());
            records.add(new RecordInfo(i, (long) i));
        }
    }

    private static void testHPUpsertInfo(int numItems) {
        System.out.println("Simulating storing upsert info in hash-map");
        HashMap<Object, RecordInfo> chm = new HashMap<>(numItems);
        for (int i = 0; i < numItems; i++) {
            chm.put(create(), new RecordInfo(i, (long) i));
        }
    }

    private static void testCHPUpsertInfo(int numItems) {
        System.out.println("Simulating storing upsert info in concurrent hash-map");
        ConcurrentHashMap<Object, RecordInfo> chm = new ConcurrentHashMap<>(numItems);
        for (int i = 0; i < numItems; i++) {
            chm.put(create(), new RecordInfo(i, (long) i));
        }
    }

    private static PrimaryKey create() {
        Object[] uuid = new Object[]{UUID.randomUUID().toString()};
        return new PrimaryKey(uuid);
    }

    static class RecordInfo {
        private int _docId;
        private Comparable _comparisonColumn;

        public RecordInfo(int docId, @Nullable Comparable comparisonColumn) {
            _docId = docId;
            _comparisonColumn = comparisonColumn;
        }
    }

    static class PrimaryKey {
        private final Object[] _values;

        public PrimaryKey(Object[] values) {
            _values = values;
        }

        public int hashCode() {
            return Arrays.hashCode(_values);
        }
    }
}
