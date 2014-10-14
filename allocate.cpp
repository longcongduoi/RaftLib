/**
 * allocate.cpp - 
 * @author: Jonathan Beard
 * @version: Tue Sep 16 20:20:06 2014
 * 
 * Copyright 2014 Jonathan Beard
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <cassert>
#include <thread>

#include "fifo.hpp"

#include "allocate.hpp"
#include "port_info.hpp"
#include "map.hpp"
#include "portexception.hpp"

Allocate::Allocate( Map &map, volatile bool &exit_alloc ) : 
   source_kernels( map.source_kernels ),
   all_kernels(    map.all_kernels ),
   exit_alloc( exit_alloc )
{
}

Allocate::~Allocate()
{
   for( FIFO *f : allocated_fifo )
   {
      delete( f );
   }
}

void
Allocate::waitTillReady()
{
   while( ! ready );
}

void
Allocate::setReady()
{
   ready = true;
}

void
Allocate::initialize( PortInfo *src, PortInfo *dst, FIFO *fifo )
{
   assert( fifo != nullptr );
   if( src != nullptr )
   {
      if( src->getFIFO() != nullptr )
      {
      throw PortDoubleInitializeException( 
         "Source port \"" + src->my_name + "\" already initialized!" );
      }
   }
   if( dst != nullptr && dst->getFIFO() != nullptr )
   {
      throw PortDoubleInitializeException( 
         "Destination port \"" + dst->my_name +  "\" already initialized!" );
   }
   if( src != nullptr )
   {
      src->setFIFO( fifo );
   }
   if( dst != nullptr )
   {
      dst->setFIFO( fifo );
   }
   /** NOTE: this list simply speeds up the monitoring if we want it **/
   allocated_fifo.insert( fifo ); 
}

void
Allocate::reinitialize( PortInfo *src, PortInfo *dst, FIFO *new_fifo )
{
   assert( new_fifo != nullptr );
   assert( src      != nullptr );
   assert( dst      != nullptr );

   allocated_fifo.insert( new_fifo );
  
   if( dst != nullptr )
   {
      FIFO *old_fifo( dst->getFIFO() );
      src->setFIFO( new_fifo );
      /** dst kernel keeps draining **/
      while( old_fifo->size() > 0 )
      {
         std::this_thread::yield();
      }
      /** 
       * old fifo is empty, if dst ! null 
       * then set to new fifo otherwise probably 
       * TCP link.
       */
      dst->setFIFO( new_fifo );
      /* now the FIFO *old_fifo is empty, erase from set */
      allocated_fifo.erase( old_fifo );
      /** swap complete, delete the old fifo to complete **/
      //delete( old_fifo );
   }
   else
   {
      FIFO *old_fifo( src->getFIFO() );
      src->setFIFO( new_fifo );
      /** dst kernel keeps draining **/
      while( old_fifo->size() > 0 )
      {
         std::this_thread::yield();
      }
      /* now the FIFO *old_fifo is empty, erase from set */
      allocated_fifo.erase( old_fifo );
      /** swap complete, delete the old fifo to complete **/
      delete( old_fifo );
   }

}
