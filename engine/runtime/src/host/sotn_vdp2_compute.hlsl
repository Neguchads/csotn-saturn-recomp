// Composição inteira e discreta; não usa blending aproximado do driver.
ByteAddressBuffer memory : register(t0, space0);
RWByteAddressBuffer outputPixels : register(u0, space1);
cbuffer Parameters : register(b0, space2) { uint4 settings[33]; };
uint param(uint i) { return settings[i/4][i%4]; }
uint desc(uint n, uint i) { return param(4+n*32+i); }
uint byteAt(uint a) { return memory.Load(a & ~3u) >> ((a & 3)*8) & 255; }
uint wordAt(uint a) { return byteAt(a)*256+byteAt(a+1); }
uint reg(uint a) { return wordAt(0x81000+a); }
uint vb(uint a) { return byteAt(a & 0x7FFFF); }
uint vw(uint a) { a &= 0x7FFFE; return vb(a)*256+vb(a+1); }
uint vl(uint a) { return (vw(a)<<16)|vw(a+2); }
uint rgb555(uint v) { return ((v & 31)<<19) | (((v>>5)&31)<<11) | (((v>>10)&31)<<3); }
struct Pix { uint rgb, prio, ratio, cc, offset, offset_b, spr, scc, msb, shade, code, direct; };
Pix emptyPixel() { return (Pix)0; }
uint cram(uint i, out uint msb) {
    uint mode=(reg(0xE)>>12)&3;
    if (mode==2) {
        uint a=0x80000+(i&0x3FF)*4;
        uint v=wordAt(a)*65536+wordAt(a+2); msb=v>>31;
        return ((v&255)<<16) | (v&0xFF00) | ((v>>16)&255);
    }
    uint v=wordAt(0x80000+(i&(mode==1?0x7FF:0x3FF))*2); msb=v>>15;
    return rgb555(v);
}
bool dot(uint n,uint base,uint x,uint y,uint width,uint pal,inout Pix p) {
    uint i=y*width+x, v=0, colors=desc(n,6), opaque=desc(n,1);
    if (colors==0) { v=vb(base+i/2); v=(i&1)?v&15:v>>4; }
    else if (colors==1) v=vb(base+i);
    else if (colors==2) v=vw(base+i*2)&0x7FF;
    else {
        uint w=colors==3?vw(base+i*2):(vw(base+i*4)<<16)|vw(base+i*4+2);
        uint mask=colors==3?0x8000:0x80000000;
        if (!(w&mask) && !opaque) return false;
        p.rgb=colors==3?rgb555(w):((w&255)<<16)|(w&0xFF00)|((w>>16)&255);
        p.msb=(w&mask)!=0; p.direct=1; p.code=7; return true;
    }
    if (!v && !opaque) return false;
    p.code=(v>>1)&7;
    uint index=colors==0?pal*16+v:colors==1?(pal&0x70)*16+v:v;
    p.rgb=cram(index+desc(n,17),p.msb); return true;
}
bool nbgPixel(uint n,uint x,uint y,inout Pix p) {
    uint ox=desc(n,18),oy=desc(n,19),dx=desc(n,20),row=y,control=desc(n,31);
    if(control) {
        uint mw=(control&15)+1,mh=((control>>4)&15)+1;
        x-=x%mw;y-=y%mh;row=y;
        uint flags=control>>8&14,step=control>>12&3;
        if(flags) {
            uint fields=uint((flags&2)!=0)+uint((flags&4)!=0)+uint((flags&8)!=0);
            uint b=0xA0+n*4;
            uint table=((reg(b)&7)<<17)|((reg(b+2)&0xFFFE)<<1);
            uint address=table+(y>>step)*fields*4;
            if(flags&2){ox+=(vl(address)>>8)&0x7FFFF;address+=4;}
            if(flags&4){oy+=(vl(address)>>8)&0x7FFFF;row&=(1u<<step)-1;address+=4;}
            if(flags&8)dx=(vl(address)>>8)&0x7FF;
        }
    }
    uint sx=(ox+x*dx)>>8,sy=(oy+row*desc(n,21))>>8;
    if (desc(n,2)) {
        p.spr=desc(n,15); p.scc=desc(n,16);
        return dot(n,desc(n,13),sx&(desc(n,11)-1),sy&(desc(n,12)-1),desc(n,11),desc(n,14),p);
    }
    uint pw=desc(n,8)*512, ph=desc(n,9)*512;
    sx &= pw*2-1; sy &= ph*2-1;
    uint plane=(sy/ph)*2+sx/pw, ix=sx&(pw-1), iy=sy&(ph-1);
    uint page=(iy>>9)*desc(n,8)+(ix>>9), cs=desc(n,3)?16:8, cpr=512/cs;
    uint addr=desc(n,27+plane)+page*desc(n,10)+(((iy&511)/cs)*cpr+(ix&511)/cs)*(desc(n,4)?2:4);
    uint chr,pal,hf=0,vf=0,supp=desc(n,7);
    if (desc(n,4)) {
        uint w=vw(addr); p.spr=(supp>>9)&1; p.scc=(supp>>8)&1;
        pal=desc(n,6)==0?((w>>12)&15)|((supp>>1)&0x70):((w>>8)&0x70);
        if (!desc(n,5)) {
            hf=(w&0x400)!=0; vf=(w&0x800)!=0;
            chr=desc(n,3)?((w&0x3FF)<<2)|(supp&3)|((supp&0x1C)<<10):(w&0x3FF)|((supp&0x1F)<<10);
        } else chr=desc(n,3)?((w&0xFFF)<<2)|(supp&3)|((supp&0x10)<<10):(w&0xFFF)|((supp&0x1C)<<10);
    } else {
        uint w=vw(addr); hf=(w&0x4000)!=0; vf=(w&0x8000)!=0;
        p.spr=(w>>13)&1; p.scc=(w>>12)&1; pal=w&0x7F; chr=vw(addr+2)&0x7FFF;
    }
    uint px=ix&(cs-1), py=iy&(cs-1);
    if(hf) px=cs-1-px; if(vf) py=cs-1-py;
    const uint cellBytes[5]={32,64,128,128,256};
    uint base=chr*32;
    if(desc(n,3)) base+=((py>>3)*2+(px>>3))*cellBytes[desc(n,6)];
    return dot(n,base,px&7,py&7,8,pal,p);
}
struct Win { bool on; int xs,xe; };
Win windowLine(uint n,uint y) {
    uint b=0xC0+n*8; int xs=reg(b)&0x3FF,xe=reg(b+4)&0x3FF;
    uint lw=reg(0xD8+n*4);
    if(lw&0x8000) {
        uint a=(((lw&7)<<16|reg(0xDA+n*4))&0x7FFFE)*2+y*4;
        xs=vw(a)&0x3FF; xe=vw(a+2)&0x3FF;
    }
    if(!(reg(0)&2)) { xs>>=1; xe>>=1; }
    uint ys=reg(b+2)&0x3FF,ye=reg(b+6)&0x3FF;
    if(!param(2)) {ys&=0x1FF;ye&=0x1FF;}
    Win w; w.on=y>=ys && y<=ye; w.xs=xs; w.xe=xe; return w;
}
bool windowed(uint ctl,Win a,Win b,uint x,bool spwin) {
    bool any=false,all=true,used=false;
    Win w[2]={a,b};
    [unroll] for(uint i=0;i<2;++i) {
        if(!(ctl>>(i*2+1)&1)) continue;
        bool inside=w[i].on && int(x)>=w[i].xs && int(x)<=w[i].xe;
        bool area=(ctl>>(i*2)&1)?!inside:inside;
        any|=area; all&=area; used=true;
    }
    if(ctl&0x20) {bool area=spwin!=((ctl&0x10)!=0);any|=area;all&=area;used=true;}
    return used && ((ctl&0x80)?all:any);
}
uint prioOf(uint i) {return reg(0xF0+i/2*2)>>((i&1)*8)&7;}
uint ratioOf(uint i) {return reg(0x100+i/2*2)>>((i&1)*8)&31;}
uint spritePixel(uint d,inout Pix p) {
    uint sp=reg(0xE0),type=sp&15; bool win=(sp&16)!=0,mixed=(sp&32)!=0;
    if(!d) return 0;
    uint msb=0,kind=1;
    if(mixed && (d&0x8000)) {p.rgb=rgb555(d);p.prio=prioOf(0);p.ratio=ratioOf(0);msb=1;}
    else {
        const uint prShift[16]={14,13,14,13,13,12,12,12,7,7,6,0,7,7,6,0};
        const uint prMask[16]={3,7,1,3,3,7,7,7,1,1,3,0,1,1,3,0};
        const uint ccShift[16]={11,11,11,11,10,11,10,9,0,6,0,6,0,6,0,6};
        const uint ccMask[16]={7,3,7,3,7,1,3,7,0,1,0,3,0,1,0,3};
        const uint dcMask[16]={0x7FF,0x7FF,0x7FF,0x7FF,0x3FF,0x7FF,0x3FF,0x1FF,0x7F,0x3F,0x3F,0x3F,0xFF,0xFF,0xFF,0xFF};
        if(type>=8) d&=255;
        uint dc=d&dcMask[type]; bool shadow=type>=2 && type<=7 && !win && (d&0x8000);
        if(dc==dcMask[type]-1) kind=2;
        else if(shadow && !(d&0x7FFF)) {if(!(reg(0xE2)&0x100)) return 0;kind=2;}
        else if(!dc) return 0;
        p.shade=shadow; p.prio=prioOf((d>>prShift[type])&prMask[type]);
        p.ratio=ratioOf((d>>ccShift[type])&ccMask[type]);
        p.rgb=cram(dc+(((reg(0xE6)>>4)&7)<<8),msb);
    }
    uint mode=(sp>>12)&3,num=(sp>>8)&7;
    p.cc=(mode==0?p.prio<=num:mode==1?p.prio==num:mode==2?p.prio>=num:msb!=0) && (reg(0xEC)&64);
    p.offset=reg(0x110)>>6&1;p.offset_b=reg(0x112)>>6&1;
    return p.prio?kind:0;
}
uint average(uint a,uint b) {return (a+b-((a^b)&0x010101))>>1;}
uint applyOffset(uint rgb,uint selected) {
    uint base=selected?0x11A:0x114,outColor=0;
    [unroll] for(int i=0;i<3;++i) {
        int offset=int((reg(base+i*2)&0x1FF)<<23)>>23;
        int c=int(rgb>>(16-i*8)&255)+offset;
        outColor=(outColor<<8)|uint(clamp(c,0,255));
    }
    return outColor;
}
[numthreads(8,8,1)]
void main(uint3 tid:SV_DispatchThreadID) {
    uint x=tid.x,y=tid.y,w=param(0),h=param(1);if(x>=w||y>=h)return;
    Win wa=windowLine(0,y),wb=windowLine(1,y);
    uint bk=((reg(0xAC)&7)<<16|reg(0xAE))*2;
    Pix back=emptyPixel();back.rgb=rgb555(vw(bk+((reg(0xAC)&0x8000)?(param(2)?y>>1:y)*2:0)));
    back.offset=reg(0x110)>>5&1;back.offset_b=reg(0x112)>>5&1;back.ratio=reg(0x10E)>>8&31;
    Pix layers[5];[unroll]for(uint k=0;k<5;++k)layers[k]=emptyPixel();
    uint data=memory.Load(0x81200+(y*w+x)*4),sp=reg(0xE0),type=sp&15;
    bool spwin=(sp&16)&&type>=2&&type<=7&&(data&0x8000)&&!(sp&32);
    uint shadow=0;
    if(!windowed(reg(0xD4)>>8,wa,wb,x,spwin)) {
        uint kind=spritePixel(data,layers[0]);
        if(kind==2){shadow=layers[0].prio;layers[0]=emptyPixel();}
    }
    [unroll]for(uint n=0;n<4;++n) {
        uint ctl=n<2?reg(0xD0)>>(n*8):reg(0xD2)>>((n-2)*8);
        if(!desc(n,0)||!desc(n,22)||windowed(ctl&255,wa,wb,x,spwin))continue;
        Pix p=emptyPixel();if(!nbgPixel(n,x,y,p))continue;
        p.prio=desc(n,22);uint special=reg(0xEA)>>(n*2)&3;
        if(special==1)p.prio=(p.prio&6)|p.spr;
        uint codes=reg(0x26)>>((reg(0x24)>>n&1)*8)&255;bool match=(codes>>p.code&1)!=0;
        if(special==2)p.prio=(p.prio&6)|uint(!p.direct&&p.spr&&match);
        uint cc=reg(0xEE)>>(n*2)&3;
        p.cc=desc(n,24)&&(cc==1?p.scc!=0:cc==2?(p.scc&&match):cc==3?p.msb!=0:true);
        p.ratio=desc(n,23);p.offset=desc(n,25);p.offset_b=desc(n,26);layers[n+1]=p;
    }
    int top=-1,second=-1,third=-1;
    [unroll]for(int i=0;i<5;++i) {
        if(!layers[i].prio)continue;
        if(top<0||layers[i].prio>layers[top].prio){third=second;second=top;top=i;}
        else if(second<0||layers[i].prio>layers[second].prio){third=second;second=i;}
        else if(third<0||layers[i].prio>layers[third].prio)third=i;
    }
    Pix t=back,u=back;if(top>=0)t=layers[top];if(second>=0)u=layers[second];
    uint ctrl=reg(0xEC);bool extended=(ctrl&0x400)&&!(reg(0)&6);
    if(extended&&u.cc)u.rgb=average(u.rgb,third>=0?layers[third].rgb:back.rgb);
    uint topBit=top==0?5:uint(top-1);
    if(top>=0&&(reg(0xE8)>>topBit&1)) {
        uint addr=((reg(0xA8)&7)<<16|reg(0xAA))*2+((reg(0xA8)&0x8000)?y*2:0),msb;
        Pix linePixel=emptyPixel();linePixel.rgb=cram(vw(addr)&0x7FF,msb);linePixel.ratio=reg(0x10E)&31;linePixel.cc=ctrl&0x20;
        if(extended&&linePixel.cc){u.rgb=average(u.rgb,linePixel.rgb);u.ratio=linePixel.ratio;}else u=linePixel;
    }
    uint rgb=t.rgb;
    if(top>=0&&t.cc&&!windowed(reg(0xD6)>>8,wa,wb,x,spwin)) {
        uint ratio=(ctrl&0x200)?u.ratio:t.ratio;rgb=0;
        [unroll]for(int shift=16;shift>=0;shift-=8) {
            uint a=t.rgb>>shift&255,b=u.rgb>>shift&255;
            uint c=(ctrl&0x100)?min(a+b,255):(a*(31-ratio)+b*(ratio+1))>>5;rgb|=c<<shift;
        }
    }
    if(t.offset)rgb=applyOffset(rgb,t.offset_b);
    bool shaded=top==0?t.shade!=0:shadow&&shadow>=t.prio&&(reg(0xE2)>>(top>=0?topBit:5)&1);
    if(shaded)rgb=rgb>>1&0x7F7F7F;
    outputPixels.Store((y*w+x)*4,rgb);
}
