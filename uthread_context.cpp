/*
* ==============================================
*        Author: linushuang   Email:linushuang@tencent.com
*        Create Time: 2016-10-11 20:27
*        Last modified: 2016-10-11 20:27
*        Filename: uthread_context.cpp
*        Description: 
* ==============================================
*/

#include "uthread_context.h" 

extern "C"  int save_context(jmp_buf jbf);
extern "C"  void restore_context(jmp_buf jbf, int ret);
extern "C"  void replace_esp(jmp_buf jbf, void* esp); 

uthread_context * uthread_sched::_cur_thread(NULL);

//����uthread_pid , ���ط�0��
uint32_t uthread_sched::GetNextPid()
{
    _next_pid ++;
    if( _next_pid )
        return _next_pid;
    else       
    {
        _next_pid++;
        return _next_pid;
    }
}

uint32_t uthread_sched::CreateUthread(UthreadEntry func, void *arg  )
{
    uint32_t pid = GetNextPid();
    uthread_context * puthread = new uthread_context;
    puthread->_uthread_pid = pid; 
    char * pstack = new char[_default_stack_size];
    puthread->_stack_addr  = pstack; 
    puthread->_stack_len   = _default_stack_size;
    puthread->_uthread_entry = func;
    puthread->_arg           = arg ;
    puthread->_state         = UTHREAD_STATE_RUNABLE;
    _thread_map[pid] = puthread;
    //=====���ԣ�����ջ
    char szBuf[1024];
    for(int i=0; i<100; i++ )
        szBuf[i] = i;
    int len = sizeof(szBuf);
    //save context, ����������
    int iret = save_context(puthread->_ctx); 
    //int iret = setjmp(puthread->_ctx );
    if( iret == 0 )
    {
        size_t addr = ((size_t)pstack) +  _default_stack_size;
        printf("create a new thread:%u, stack_addr:%p|%p, len:%u\n", pid, addr, pstack, _default_stack_size);
        //�滻jmp_buf�е�esp/rsp
        //puthread->_ctx->__jmpbuf[1] = addr;
        replace_esp(puthread->_ctx, (void*)addr);
        _runable_list.push_back(puthread );  
        return pid;
    }  
    else //iret != 0  ��ʾ��longjmp�������������ʾЭ�̿��Կ�ʼ����
    {  //����ֱ���� puthreadҲ���ԣ���, ������
       //_cur_thread ������ �������_cur_thread �Ǿ�̬��Ա��������ȫ�ֱ������ԣ��������ͨ��Ա��������
       //why : ��ĺ����� �и���������this, Ҳ���ǵ�һ������this, ����this�Ǵ���ջ�ϣ�ͬ���� puthread����ʱ������Ҳ��ջ��
       // ע�⣺���ﴴ���̣߳������ջ�Ǹ��̵߳�ջ�����뵽������õ����̵߳�ջ���ռ䲻һ���������أ���this->xxx�϶�Ҳ�����⣬
       // Ҳ���Ǵ��뵽���������ǰ��������κγ�����ջ�ϵı���
       // ǰ�����ʱ����puthread ,�������ɺϣ�ֻ�� ǡ�� puthread��ֵ�ڼĴ����У�����������ʱ�������ˣ� ��������� for ���Ӵ���ջ�󣬾��ܲ�����
        uthread_context * p  = _cur_thread;
        printf("_cur_thread:%p\n", _cur_thread);        
        if(p->_uthread_entry ) 
             p->_uthread_entry( p->_arg  );
        //Э��ִ���꺯��, ����
        p->_state = UTHREAD_STATE_DONE;
        //_thread_map.erase(puthread->_uthread_pid); 
        delete p ;
        SchedThread();  
        return 0;
    }
    return 0; 
}

//�����߳�����
void uthread_sched::SchedThread()
{
    uthread_context *  puthread  = NULL; 
    if( _runable_list.size() >  0 )     
    { //����п������̣߳���ѡ��һ��������
        puthread = *_runable_list.begin();
        _runable_list.pop_front();
    }
    else
    {//���û�п������߳�,��yield�߳�ѡ��һ��������
        puthread = *_yield_list.begin();
        _yield_list.pop_front();
    }
    
    _cur_thread = puthread;
    _cur_thread->_state = UTHREAD_STATE_RUNNING;
    
    //�ָ�_cur_thread��������
    printf("long jump to pid:%u\n", _cur_thread->_uthread_pid);
    //longjmp(_cur_thread->_ctx, 1);
    restore_context(_cur_thread->_ctx, 1);
}

//��ǰ�߳���������
void uthread_sched::Yield() 
{
    printf("Yield\n");
    if(_cur_thread != NULL )
    {
        printf("pid:%u Yield\n", _cur_thread->_uthread_pid );
        _yield_list.push_back(_cur_thread);
        uthread_context *puthread  = _cur_thread;

        int iret = save_context(puthread->_ctx);  

        //int iret = setjmp(puthread->_ctx);  
        if(iret == 0 )
        {
            printf("pid:%u  Yield SchedThread\n", puthread->_uthread_pid);
            SchedThread();
        }
    }
}


