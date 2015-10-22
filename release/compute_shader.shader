
struct DataElem
{
    float val;
};

StructuredBuffer<DataElem> 		Buffer0 : register(t0);
RWStructuredBuffer<DataElem> 	BufferOut : register(u0);

groupshared float SharedMem[THREAD_X];

// groupIndex = Thread within groupID
// groupID = Number of the group

[numthreads(THREAD_X,THREAD_Y,THREAD_Z)]
void  entry_point(uint3 dispatchThreadID : SV_DispatchThreadID,
				  uint3 GroupThreadID : SV_GroupThreadID,
				  uint3 groupID : SV_GroupID,
				  uint groupIndex : SV_GroupIndex)
{
	SharedMem[GroupThreadID.x]=Buffer0[dispatchThreadID.x].val;
	
	GroupMemoryBarrierWithGroupSync();
	
	for (uint s=THREAD_X/2; s>0; s>>=1)
	{
		if (groupIndex<s)
		{
			SharedMem[groupIndex]+=SharedMem[groupIndex+s];
		}
		GroupMemoryBarrierWithGroupSync();
	}
	
	if (groupIndex==0) BufferOut[groupID.x].val=SharedMem[0];
}
