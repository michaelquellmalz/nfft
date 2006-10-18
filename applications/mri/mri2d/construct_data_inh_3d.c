#include "nfft3.h"
#include "util.h"
#include "math.h"
#include "limits.h"

/**
 * construct 
 */
void construct(char * file, int N, int M)
{
  int j,k,l;                  /* some variables */
  double real,imag;
  double w;
  double time,min_time,max_time,min_inh,max_inh;
  mri_inh_3d_plan my_plan;  
  FILE *fp,*fout,*fi,*finh,*ftime;
  int my_N[3],my_n[3];
  int flags = PRE_PHI_HUT| PRE_PSI |MALLOC_X| MALLOC_F_HAT|
                      MALLOC_F| FFTW_INIT| FFT_OUT_OF_PLACE|
                      FFTW_MEASURE| FFTW_DESTROY_INPUT;

  double Ts;
  double W;
  int N3;
  int m=2;
  double sigma = 1.25;

  ftime=fopen("readout_time.dat","r");
  finh=fopen("inh.dat","r");

  min_time=INT_MAX; max_time=INT_MIN;
  for(j=0;j<M;j++)
  {
    fscanf(ftime,"%le ",&time);
    if(time<min_time)
      min_time = time;
    if(time>max_time)
      max_time = time;
  }

  fclose(ftime);
  
  Ts=(min_time+max_time)/2.0;

  min_inh=INT_MAX; max_inh=INT_MIN;
  for(j=0;j<N*N;j++)
  {
    fscanf(finh,"%le ",&w);
    if(w<min_inh)
      min_inh = w;
    if(w>max_inh)
      max_inh = w;
  }
  fclose(finh);

  N3=ceil((NFFT_MAX(fabs(min_inh),fabs(max_inh))*(max_time-min_time)/2.0+m/(2*sigma))*4*sigma);

  W= NFFT_MAX(fabs(min_inh),fabs(max_inh))/(0.5-((double)m)/N3);

  my_N[0]=N; my_n[0]=ceil(N*sigma);
  my_N[1]=N; my_n[1]=ceil(N*sigma);
  my_N[2]=N3; my_n[2]=ceil(N3*sigma);
  
  /* initialise nfft */ 
  mri_inh_3d_init_guru(&my_plan, my_N, M, my_n, m, sigma, flags,
                      FFTW_MEASURE| FFTW_DESTROY_INPUT);

  ftime=fopen("readout_time.dat","r");
  fp=fopen("knots.dat","r");
  
  for(j=0;j<my_plan.M_total;j++)
  {
    fscanf(fp,"%le %le",&my_plan.plan.x[3*j+0],&my_plan.plan.x[3*j+1]);
    fscanf(ftime,"%le ",&my_plan.plan.x[3*j+2]);
    my_plan.plan.x[3*j+2] = (my_plan.plan.x[3*j+2]-Ts)*W/N3;
  }
  fclose(fp);
  fclose(ftime);

  finh=fopen("inh.dat","r");
  for(j=0;j<N*N;j++)
  {
    fscanf(finh,"%le ",&my_plan.w[j]);
    my_plan.w[j]/=W;
  }
  fclose(finh);


  fi=fopen("input_f.dat","r");
  for(j=0;j<N*N;j++)
  {
    fscanf(fi,"%le ",&real);
    my_plan.f_hat[j] = real*cexp(2.0*I*PI*Ts*my_plan.w[j]*W);
  }
  
  if(my_plan.plan.nfft_flags & PRE_PSI)
    nfft_precompute_psi(&my_plan.plan);

  mri_inh_3d_trafo(&my_plan);

  fout=fopen(file,"w");
  
  for(j=0;j<my_plan.M_total;j++)
  {
    fprintf(fout,"%le %le %le %le\n",my_plan.plan.x[3*j+0],my_plan.plan.x[3*j+1],creal(my_plan.f[j]),cimag(my_plan.f[j]));
  }

  fclose(fout);

  mri_inh_3d_finalize(&my_plan);
}

int main(int argc, char **argv)
{ 
  if (argc <= 3) {
    printf("usage: ./construct_data_inh_3d FILENAME N M\n");
    return 1;
  }
  
  construct(argv[1],atoi(argv[2]),atoi(argv[3]));
  
  return 1;
}
