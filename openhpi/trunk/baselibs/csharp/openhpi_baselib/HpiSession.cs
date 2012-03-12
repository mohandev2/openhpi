using System;
using System.Collections.Generic;
using System.Threading;


namespace org {
namespace openhpi {


internal class HpiSession
{
    private static long cnt = 0;


    private long                   local_sid;
    private long                   remote_sid;
    private HpiDomain              domain;
    private LinkedList<HpiMarshal> marshals;


    public HpiSession( HpiDomain domain )
    {
        // TODO investigate overflow issue
        this.local_sid  = Interlocked.Increment( ref cnt );
        this.remote_sid = 0;
        this.domain     = new HpiDomain( domain );
        this.marshals   = new LinkedList<HpiMarshal>();
    }

    public void Close()
    {
        lock ( marshals )
        {
            while( marshals.Count > 0 ) {
                HpiMarshal m = marshals.First.Value;
                marshals.RemoveFirst();
                m.Close();
            }
        }
    }

    public long GetLocalSid()
    {
        return local_sid;
    }

    public long GetRemoteSid()
    {
        return remote_sid;
    }

    public void SetRemoteSid( long remote_sid )
    {
        this.remote_sid = remote_sid;
    }

    public long GetRemoteDid()
    {
        return domain.GetRemoteDid();
    }

    public HpiMarshal GetMarshal()
    {
        HpiMarshal m;

        lock ( marshals )
        {
            if ( marshals.Count > 0 ) {
                m = marshals.First.Value;
                marshals.RemoveFirst();
            } else {
                m = new HpiMarshal();
                bool rc = m.Open( domain.GetRemoteHost(), domain.GetRemotePort() );
                if ( !rc ) {
                    m = null;
                }
            }
            if ( m != null ) {
                m.Reset();
            }
        }

        return m;
    }

    public void PutMarshal( HpiMarshal m )
    {
        m.Reset();
        lock ( marshals )
        {
            marshals.AddFirst( m );
        }
    }
};


}; // namespace openhpi
}; // namespace org

