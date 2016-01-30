logFolder = r'C:\Users\mikfig\Documents\Visual Studio 2015\Projects\sdl_test\sdl_test'
logs = []

for i in range(10):
    logs.append([])


def getNum(s):
    first = 0
    while '0' > s[first] or s[first] > '9':
        first += 1
    last = first
    while '0' <= s[last] and s[last] <= '9':
        last += 1
    return int(s[first:last]), last


timeStampStr = 'TS - '
postTimeStampStr = ' - '
for nodeNum in range(10):
    nodePrefixStr = 'Node #%d\t' % nodeNum

    with open(r'%s\node%d.txt' % (logFolder, nodeNum), 'r') as fp:
        line = fp.readline()
        currentBlock = ''
        currentTimestamp = 0
        while len(line) > 0:
            if line[:len(timeStampStr)] == timeStampStr:
                if len(currentBlock) > 0:
                    logs[nodeNum].append((currentTimestamp, currentBlock))
                line = line[len(timeStampStr):]
                currentTimestamp, postTimestampStart = getNum(line)
                postTimestampStart += len(postTimeStampStr)
                currentBlock = nodePrefixStr + line[postTimestampStart:]
            else:
                currentBlock += line
            line = fp.readline()

nodeLogIndex = [0] * 10

with open(r'%s\nodes_collated.txt' % logFolder, 'w') as fp:
    while True:
        smallestTimestamp = float('inf')
        nodeToOutput = 0
        haveLogEntriesLeft = False
        for i in range(10):
            if nodeLogIndex[i] < len(logs[i]):
                haveLogEntriesLeft = True
                if logs[i][nodeLogIndex[i]][0] < smallestTimestamp:
                    smallestTimestamp = logs[i][nodeLogIndex[i]][0]
                    nodeToOutput = i
        if not haveLogEntriesLeft:
            break
        fp.write(logs[nodeToOutput][nodeLogIndex[nodeToOutput]][1])
        nodeLogIndex[nodeToOutput] += 1
